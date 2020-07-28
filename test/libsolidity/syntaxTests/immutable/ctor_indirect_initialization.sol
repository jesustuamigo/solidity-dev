contract C {
    uint immutable x;
    constructor() {
        initX();
    }

    function initX() internal {
        x = 3;
    }
}
// ----
// TypeError 1581: (119-120): Immutable variables can only be initialized inline or assigned directly in the constructor.
