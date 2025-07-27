.section .note.pccssystems, "a"
.align 4

# 1. Header
.long 12                # namesz (including null)
.long 64                # descsz (including nulls)
.long 1                 # type (custom)

# 2. Name (must be padded to align 4)
.asciz "Strapicarus"
.align 4

# 3. Description
.asciz "Author: Strapicarus\n"
.asciz "Version: 0.3.1\n"
.asciz "Developer: Strapicarus\n"

.align 4

.section .note.GNU-stack,"",@progbits