contract C
{
    function f() public
    {
        int x = --(,);
    }
}
// ----
// TypeError 9767: (59-64): Built-in unary operator -- cannot be applied to type tuple(,).
