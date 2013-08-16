// v8-shell.m.cpp                                                     -*-C++-*-

#include <v8.h>

#include <iostream>
#include <iterator>

#define CHECK_RETURN(tryCatch)                                                \
    do {                                                                      \
        if (tryCatch.HasCaught()) {                                           \
            v8::Handle<v8::Message> message = tryCatch.Message();             \
            formatMessage(std::cerr, tryCatch.Message());                     \
            return -1;                                                        \
        }                                                                     \
    }                                                                         \
    while (false)

namespace {

static const char *toCString(v8::Handle<v8::Value> value)
{
    return *v8::String::Utf8Value(value);
}

static std::ostream& formatMessage(std::ostream&           stream,
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

}  // close unnamed namespace

int main(int argc, char *argv[])
{
    // Read stdin into 'input'
    std::string input;
    std::copy(std::istreambuf_iterator<char>(std::cin.rdbuf()),
              std::istreambuf_iterator<char>(),
              std::back_inserter(input));

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

    // This in combination with 'CHECK_RETURN' will do error checking
    v8::TryCatch errors;

    // Load the input string
    v8::Handle<v8::String> source = v8::String::New(input.data(),
                                                    input.length());
    CHECK_RETURN(errors);

    // Compile it
    v8::Handle<v8::Script> script = v8::Script::Compile(
                                                   source,
                                                   v8::String::New("<stdin>"));
    CHECK_RETURN(errors);

    // Evaluate it
    v8::Handle<v8::Value> result = script->Run();
    CHECK_RETURN(errors);

    // Print the result
    std::cout << toCString(result) << std::endl;

    return 0;
}
