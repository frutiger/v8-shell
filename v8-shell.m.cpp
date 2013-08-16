// v8-shell.m.cpp                                                     -*-C++-*-

#include <v8.h>

#include <iostream>
#include <iterator>

namespace {

static const char *toCString(v8::Handle<v8::Value> value)
{
    return *v8::String::Utf8Value(value);
}

static std::ostream& format(std::ostream&           stream,
                            v8::Handle<v8::Message> message)
{
    stream << toCString(message->Get()) << '\n';
    v8::Handle<v8::StackTrace> stack = message->GetStackTrace();
    if (!stack.IsEmpty()) {
        for (int i = 0; i < stack->GetFrameCount(); ++i) {
            v8::Handle<v8::StackFrame> frame = stack->GetFrame(i);
            stream << "   at ";
            if (frame->GetFunctionName()->Length()) {
                stream << toCString(frame->GetFunctionName()) << " ";
            }
            stream << "(" << toCString(frame->GetScriptName()) << ":"
                << frame->GetLineNumber()      << ":"
                << frame->GetColumn()
                << ")"
                << std::endl;
        }
    }
    return stream;
}

static std::ostream& format(std::ostream&         stream,
                            v8::Handle<v8::Value> value)
{
    return stream << toCString(value) << std::endl;
}

static int read(std::string *result, std::istream& in, std::ostream& out)
{
    result->clear();

    out << ">>> ";
    bool escape = false;
    bool append = true;
    while (true) {
        std::istream::int_type character = in.get();
        switch (character) {
            case -1: {
                out << "\n";
            } return -1;

            case '\n': {
                if (escape) {
                    out << "... ";
                    escape = false;
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

static int evaluate(v8::Handle<v8::Value>   *result,
                    v8::Handle<v8::Message> *errorMessage,
                    const std::string&       input)
{
    // This will catch errors within this scope
    v8::TryCatch tryCatch;

    // Load the input string
    v8::Handle<v8::String> source = v8::String::New(input.data(),
                                                    input.length());
    if (tryCatch.HasCaught()) {
        *errorMessage = tryCatch.Message();
        return -1;
    }

    // Compile it
    v8::Handle<v8::Script> script = v8::Script::Compile(
                                                   source,
                                                   v8::String::New("<stdin>"));
    if (tryCatch.HasCaught()) {
        *errorMessage = tryCatch.Message();
        return -1;
    }

    // Evaluate it
    *result = script->Run();
    if (tryCatch.HasCaught()) {
        *errorMessage = tryCatch.Message();
        return -1;
    }

    return 0;
}

}  // close unnamed namespace

int main(int argc, char *argv[])
{
    // Initialize global v8 variables
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::V8::SetCaptureStackTraceForUncaughtExceptions(
                                                    true,
                                                    0x100,
                                                    v8::StackTrace::kDetailed);

    // Stack-local storage
    v8::HandleScope handles(isolate);

    // Create and enter a context
    v8::Handle<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope      scope(context);

    std::string input;
    while (read(&input, std::cin, std::cout) == 0) {
        v8::Handle<v8::Value>   result;
        v8::Handle<v8::Message> errorMessage;
        if (evaluate(&result, &errorMessage, input)) {
            format(std::cerr, errorMessage);
        }
        else {
            std::cout << "-> ";
            format(std::cout, result);
        }
    }

    return 0;
}
