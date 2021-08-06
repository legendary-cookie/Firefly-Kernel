CC = clang++
AS = nasm
SRC_DIR = arch/$(ARCH)
INC_DIR = include
BUILD_DIR = binaries/boot


####################################
######## 64 BIT BUILD FLAGS ########
####################################
ifeq ($(ARCH), x86_64)
$(info = Firefly target architecture: x86_64 =)
$(info )

CXX_FLAGS =						\
	-I./include/		 		\
	-I./include/stl/			\
	-target x86_64-unknown-elf	\
	-m64						\
	-mcmodel=kernel				\
	-std=c++20   				\
	-Wall						\
	-Wextra 					\
	-pedantic 					\
	-Werror 					\
	-g 							\
	-nostdlib 					\
	-fno-builtin 				\
	-fno-PIC 					\
	-mno-red-zone 				\
	-fno-stack-check 			\
	-fno-stack-protector 		\
	-fno-omit-frame-pointer 	\
	-ffreestanding 				\
	-fno-exceptions 			\
	-fno-rtti 					\
	-Wno-zero-length-array 		\
	-Wno-gnu

ASM_FLAGS = -f elf64 -g -F dwarf

#### KERNEL BUILD FLAGS ####
LIB_OBJS = ./include/stl/cstd.o ./include/stl/stdio.o

CXX_FILES  = $(shell find $(SRC_DIR)/ -type f -name '*.cpp')
ASM_FILES  = $(shell find $(SRC_DIR)/ -type f -name '*.asm')
CONV_FILES = $(CXX_FILES:.cpp=.cxx.o) $(ASM_FILES:.asm=.asm.o) # Convert file ext for a makefile var
OBJ_FILES  = $(addprefix $(BUILD_DIR)/,$(CONV_FILES))

### STL BUILD FLAGS ####
STL_CXX_FLAGS = -m64 -std=gnu++17 -Wall -Wextra -pedantic -Werror -g -O2 -nostdlib -fno-builtin -fno-PIC -mno-red-zone -fno-stack-check -fno-stack-protector -fno-omit-frame-pointer -ffreestanding -fno-exceptions -fno-rtti

# Note: We use . as paths are relative to the Makefile which included flags.mk
STL_SRC_DIR = .
STL_CXX_FILES = $(wildcard $(STL_SRC_DIR)/*.cpp)
STL_CXX_CSTDLIB_FILES = $(wildcard $(STL_SRC_DIR)/cstdlib/*.cpp)
endif


####################################
######## 32 BIT BUILD FLAGS ########
####################################
ifeq ($(ARCH), i386)
$(info = Firefly target architecture: i386 =)
$(info )

CXX_FLAGS =						\
	-I./include/	 			\
	-I./include/stl/			\
	-target x86_64-unknown-elf	\
	-m32						\
	-mcmodel=kernel				\
	-std=c++20   				\
	-Wall						\
	-Wextra 					\
	-pedantic 					\
	-Werror 					\
	-g 							\
	-nostdlib 					\
	-fno-builtin 				\
	-fno-PIC 					\
	-mno-red-zone 				\
	-fno-stack-check 			\
	-fno-stack-protector 		\
	-fno-omit-frame-pointer 	\
	-ffreestanding 				\
	-fno-exceptions 			\
	-fno-rtti 					\
	-Wno-zero-length-array 		\
	-Wno-gnu

ASM_FLAGS = -f elf32 -g -F dwarf

#### KERNEL BUILD FLAGS ####

LIB_OBJS = ./include/stl/cstd.o ./include/stl/stdio.o

CXX_FILES  = $(shell find $(SRC_DIR)/ -type f -name '*.cpp')
ASM_FILES  = $(shell find $(SRC_DIR)/ -type f -name '*.asm')
CONV_FILES = $(CXX_FILES:.cpp=.cxx.o) $(ASM_FILES:.asm=.asm.o) # Convert file ext for a makefile var
OBJ_FILES  = $(addprefix $(BUILD_DIR)/,$(CONV_FILES))

### STL BUILD FLAGS ####
STL_CXX_FLAGS = -m32 -std=gnu++17 -Wall -Wextra -pedantic -Werror -g -O2 -nostdlib -fno-builtin -fno-PIC -mno-red-zone -fno-stack-check -fno-stack-protector -fno-omit-frame-pointer -ffreestanding -fno-exceptions -fno-rtti

# Note: We use . as paths are relative to the Makefile which included flags.mk
STL_SRC_DIR = .
STL_CXX_FILES = $(wildcard $(STL_SRC_DIR)/*.cpp)
STL_CXX_CSTDLIB_FILES = $(wildcard $(STL_SRC_DIR)/cstdlib/*.cpp)

endif