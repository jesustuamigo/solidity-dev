==== Source: a ====
library A {}
==== Source: b ====
library A {}
==== Source: c ====
import {A} from "./a"; import {A} from "./b";
// ----
// DeclarationError: (c:23-45): Identifier already declared.
