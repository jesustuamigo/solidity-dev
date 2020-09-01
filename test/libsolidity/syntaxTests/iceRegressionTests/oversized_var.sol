contract b {
    struct c {
        uint [2 ** 253] a;
    }

    c d;

    function e() public view {
        c storage x = d;
        x.a[0];
        function()[3**44] storage fs;
    }
}
// ----
// Warning 3408: (66-69): Variable "d" covers a large part of storage and thus makes collisions likely. Either use mappings or dynamic arrays and allow their size to be increased only in small quantities per transaction.
// Warning 2332: (111-112): Type "struct b.c" covers a large part of storage and thus makes collisions likely. Either use mappings or dynamic arrays and allow their size to be increased only in small quantities per transaction.
// Warning 2332: (152-169): Type "function ()[984770902183611232881]" covers a large part of storage and thus makes collisions likely. Either use mappings or dynamic arrays and allow their size to be increased only in small quantities per transaction.
// Warning 2072: (152-180): Unused local variable.
