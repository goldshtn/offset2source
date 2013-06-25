offset2source
=============

A tool that converts module (and optional method) instruction offsets back to the .cpp source file and line number.

Usage examples:

	OffsetToSource module+0x1324
	OffsetToSource module!function+0x40

Note that for the tool to work properly, you need the module binary to reside in the %PATH% or symbol path.