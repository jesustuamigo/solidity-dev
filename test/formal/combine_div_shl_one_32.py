from opcodes import DIV, SHL, SHR
from rule import Rule
from z3 import BitVec

"""
Rule:
DIV(X, SHL(Y, 1)) -> SHR(Y, X)
Requirements:
"""

rule = Rule()

n_bits = 32

# Input vars
X = BitVec('X', n_bits)
Y = BitVec('Y', n_bits)

# Non optimized result
nonopt = DIV(X, SHL(Y, 1))

# Optimized result
opt = SHR(Y, X)

rule.check(nonopt, opt)
