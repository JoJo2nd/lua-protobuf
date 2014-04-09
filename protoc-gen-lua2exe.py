import sys
from cx_Freeze import setup, Executable

setup(
    name = "protoc-gen-lua",
    version = "0.1",
    executables = [Executable("protoc-gen-lua")]
)