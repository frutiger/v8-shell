// v8-shell.m.cpp                                                     -*-C++-*-

#include <libplatform/libplatform.h>
#include <v8.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

namespace {

#define STR(isolate, value) \
    *v8::String::Utf8Value(isolate, value)

static std::ostream& format(std::ostream&           stream,
                            v8::Handle<v8::Message> message)
{
    stream << STR(message->GetIsolate(), message->Get()) << '\n';
    auto stack = message->GetStackTrace();
    if (!stack.IsEmpty()) {
        for (int i = 0; i < stack->GetFrameCount(); ++i) {
            auto frame = stack->GetFrame(message->GetIsolate(), i);
            stream << "   at ";
            if (!frame->GetFunctionName().IsEmpty()) {
                stream << STR(message->GetIsolate(),
                              frame->GetFunctionName()) << " ";
            }
            stream << "(" << STR(message->GetIsolate(),
                                 frame->GetScriptName()) << ":"
                << frame->GetLineNumber() << ":"
                << frame->GetColumn()
                << ")"
                << std::endl;
        }
    }
    return stream;
}

static int read(v8::Isolate   *isolate,
                std::string   *result,
                std::istream&  in,
                std::ostream&  out)
{
    result->clear();

    out << ">>> ";
    bool escape = false;
    bool append = true;
    while (true) {
        auto character = in.get();
        switch (character) {
            case -1: {
                out << "\n";
            } return -1;

            case '\n': {
                if (escape) {
                    out << "... ";
                    escape = false;
                }
                else if (result->empty()) {
                    return -1;
                }
                else {
                    return 0;
                }
            } break;

            case '\\': {
                escape = true;
                append = false;
            } break;
        }
        if (append) {
            result->push_back(std::istream::traits_type::to_char_type(
                                                                   character));
        }
        append = true;
    }
}

static int evaluate(v8::Isolate             *isolate,
                    v8::Handle<v8::Context>  context,
                    v8::Handle<v8::Value>   *result,
                    v8::Handle<v8::Message> *errorMessage,
                    const std::string&       input,
                    const std::string&       inputName)
{
    // This will catch errors within this scope
    v8::TryCatch tryCatch(isolate);

    // Load the input name
    auto name = v8::String::NewFromUtf8(isolate,
                                        inputName.data(),
                                        v8::NewStringType::kNormal,
                                        inputName.length()).ToLocalChecked();
    if (tryCatch.HasCaught()) {
        *errorMessage = tryCatch.Message();
        return -1;
    }

    auto origin = v8::ScriptOrigin(isolate, name);
    // Load the input string
    auto source = v8::String::NewFromUtf8(isolate,
                                          input.data(),
                                          v8::NewStringType::kNormal,
                                          input.length()).ToLocalChecked();
    if (tryCatch.HasCaught()) {
        *errorMessage = tryCatch.Message();
        return -1;
    }

    // Compile it
    auto script = v8::Script::Compile(context, source, &origin);
    if (tryCatch.HasCaught()) {
        *errorMessage = tryCatch.Message();
        return -1;
    }

    // Evaluate it
    auto maybeResult = script.ToLocalChecked()->Run(context);
    if (tryCatch.HasCaught()) {
        *errorMessage = tryCatch.Message();
        return -1;
    }
    *result = maybeResult.ToLocalChecked();

    return 0;
}

static void evaluateAndPrint(v8::Isolate            *isolate,
                             v8::Local<v8::Context>  context,
                             std::ostream&           outStream,
                             std::ostream&           errorStream,
                             const std::string&      input,
                             const std::string&      inputName)
{
    v8::Handle<v8::Value>   result;
    v8::Handle<v8::Message> errorMessage;
    if (evaluate(isolate, context, &result, &errorMessage, input, inputName)) {
        format(errorStream, errorMessage);
    }
    else if (result != v8::Undefined(isolate)) {
        outStream << "-> " << STR(isolate, result) << '\n';
    }
}

static int readFile(std::string        *contents,
                    const std::string&  fileName)
{
    std::filebuf fileBuffer;
    fileBuffer.open(fileName, std::ios_base::in);
    if (!fileBuffer.is_open()) {
        return -1;
    }

    std::copy(std::istreambuf_iterator<char>(&fileBuffer),
              std::istreambuf_iterator<char>(),
              std::back_inserter(*contents));
    fileBuffer.close();
    return 0;
}

static int evaluateFile(v8::Isolate            *isolate,
                             v8::Local<v8::Context>  context,
                             std::ostream&      outStream,
                        std::ostream&      errorStream,
                        const std::string& programName,
                        const std::string& fileName)
{
    std::string input;
    if (readFile(&input, fileName)) {
        errorStream << "Usage: " << programName << " [<filename> | -]*\n"
                    << "Failed to open: " << fileName << std::endl;
        return -1;
    }

    evaluateAndPrint(isolate, context, outStream, errorStream, input, fileName);
    return 0;
}

static void beginReplLoop(v8::Isolate            *isolate,
                             v8::Local<v8::Context>  context,
                          std::istream&  inStream,
                          std::ostream&  outStream,
                          std::ostream&  errorStream)
{
    std::string input;
    int         command = 0;
    while (read(isolate, &input, inStream, outStream) == 0) {
        std::ostringstream oss;
        oss << "<stdin:" << ++command << ">";
        evaluateAndPrint(isolate,
                         context,
                         outStream,
                         errorStream,
                         input,
                         oss.str());
    }
}

static void print(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    for (int i = 0; i < args.Length(); ++i) {
        std::cout << (i == 0 ? "" : ",") << STR(args.GetIsolate(), args[i]);
    }
    std::cout << std::endl;

    args.GetReturnValue().SetUndefined();
}

static void read(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() == 0) {
        args.GetIsolate()->ThrowException(
            v8::Exception::Error(
                v8::String::NewFromUtf8Literal(
                                 args.GetIsolate(),
                                 "Required argument 1 'fileName' not found")));
        args.GetReturnValue().SetUndefined();
        return;
    }

    if (!args[0]->IsString()) {
        args.GetIsolate()->ThrowException(
            v8::Exception::Error(
                v8::String::NewFromUtf8Literal(
                                args.GetIsolate(),
                                "Required argument 1 'fileName' not string")));
        args.GetReturnValue().SetUndefined();
        return;
    }

    std::string contents;
    if (readFile(&contents, STR(args.GetIsolate(),
                                args[0].As<v8::String>()))) {
        args.GetIsolate()->ThrowException(
            v8::Exception::Error(
                v8::String::NewFromUtf8Literal(args.GetIsolate(),
                                               "Failed to read file")));
        args.GetReturnValue().SetUndefined();
        return;
    }

    v8::Local<v8::String> result = v8::String::NewFromUtf8(args.GetIsolate(),
                                                           contents.data(),
                                                           v8::NewStringType::kNormal,
                                                           contents.size())
                                              .ToLocalChecked();
    args.GetReturnValue().Set(result);
}

}  // close unnamed namespace

int main(int argc, char *argv[])
{
    // Initialize global v8 variables

    auto platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());

    v8::V8::Initialize();

    v8::Isolate::CreateParams  isolateParams;
    isolateParams.array_buffer_allocator =
                             v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    auto                      *isolate = v8::Isolate::New(isolateParams);
    {
        v8::Isolate::Scope         isolateScope(isolate);

        isolate->SetCaptureStackTraceForUncaughtExceptions(
                                                        true,
                                                        0x100,
                                                        v8::StackTrace::kDetailed);

        // Stack-local storage
        v8::HandleScope handles(isolate);

        // Create and enter a context
        auto               context = v8::Context::New(isolate);
        {
            v8::Context::Scope scope(context);

            // Embed the 'print' function
            v8::Handle<v8::FunctionTemplate> printTemplate =
                                            v8::FunctionTemplate::New(isolate, &print);
            context->Global()->Set(context,
                                   v8::String::NewFromUtf8Literal(isolate, "print"),
                                   printTemplate->GetFunction(context)
                                                .ToLocalChecked())
                             .Check();

            // Embed the 'read' function
            v8::Handle<v8::FunctionTemplate> readTemplate =
                                             v8::FunctionTemplate::New(isolate, &read);
            context->Global()->Set(context,
                                   v8::String::NewFromUtf8Literal(isolate, "read"),
                                   readTemplate->GetFunction(context).ToLocalChecked())
                             .Check();

            if (argc == 1) {
                beginReplLoop(isolate, context, std::cin, std::cout, std::cerr);
            }
            else {
                for (int i = 1; i < argc; ++i) {
                    if (argv[i][0] == '-') {
                        switch (argv[i][1]) {
                            case '\0':
                                beginReplLoop(isolate,
                                              context,
                                              std::cin,
                                              std::cout,
                                              std::cerr);
                                break;

                            case 'h':
                            default:
                                std::cout
                                        << "Usage: " << argv[0] << " [<filename> | -]*"
                                        << std::endl;
                                break;
                        }
                    }
                    else {
                        evaluateFile(isolate,
                                     context,
                                     std::cout,
                                     std::cerr,
                                     argv[0],
                                     argv[i]);
                    }
                }
            }
        }
    }
    isolate->Dispose();

    v8::V8::Dispose();
    v8::V8::DisposePlatform();

    return 0;
}
