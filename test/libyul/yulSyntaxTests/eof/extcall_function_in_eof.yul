object "a" {
    code {
        function extcall() {}
    }
}

// ====
// bytecodeFormat: >=EOFv1
// ----
// ParserError 5568: (41-48): Cannot use builtin function name "extcall" as identifier name.
// ParserError 8143: (41-48): Expected keyword "data" or "object" or "}".
