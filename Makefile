
#####################################################################################################################
#                                                Project Information                                                #
#####################################################################################################################
PROJECT_NAME    :=  Windows_MiniUI
BUILD_DIR       :=  Build
OBJECT_DIR      :=  $(BUILD_DIR)/Obj
BIN_DIR         :=  $(BUILD_DIR)/Bin


#####################################################################################################################
#                                                Compiler Information                                               #
#####################################################################################################################
CC              :=  C:/gcc/mingw32/bin/g++.exe

DEF             :=  

LIBS            :=  -lbgi -lgdi32 -lcomdlg32 -luuid -loleaut32 -lole32

FLAGS           :=  -m32                                                                                            \
                    -fdiagnostics-color=always                                                                      \
                    -g

LDFLAGS         :=  


#####################################################################################################################
#                                                Source Information                                                 #
#####################################################################################################################
INCLUDES      	:=  $(OBJECT_DIR)                                                                                   \
					Graphics																						\
                    User/Inc																						\
					Draw/Common/Inc																					\
					Draw/Rgb565/Inc

SOURCES       	:=  Draw/Rgb565/Src/flint_rgb565_common.cpp															\
					Draw/Rgb565/Src/flint_rgb565_sw_line.cpp													    \
					Draw/Rgb565/Src/flint_rgb565_sw_rect.cpp														\
					Draw/Rgb565/Src/flint_rgb565_sw_ellipse.cpp														\
					User/Src/main.cpp


#####################################################################################################################
#                                                Init Variables                                                     #
#####################################################################################################################
INCLUDES        :=  $(addprefix -I,$(INCLUDES))

OBJECTS         :=  $(notdir $(SOURCES))

OBJECTS         :=  $(OBJECTS:.c=.c.o)
OBJECTS         :=  $(OBJECTS:.cpp=.cpp.o)
OBJECTS         :=  $(addprefix $(OBJECT_DIR)/,$(OBJECTS))

BUILD_DIRS      :=  $(OBJECT_DIR) $(BIN_DIR)

vpath %.c $(sort $(dir $(SOURCES)))
vpath %.cpp $(sort $(dir $(SOURCES)))


#####################################################################################################################
#                                                Makefile Rules                                                     #
#####################################################################################################################
.SUFFIXES:

all: $(BIN_DIR)/$(PROJECT_NAME).exe

$(BUILD_DIRS):
	@mkdir -p $@
	@echo Created $@

$(OBJECT_DIR)/%.c.o: %.c Makefile | $(BUILD_DIRS)
	@echo Compiling $<
	@$(CC) -c $(FLAGS) $(INCLUDES) $(DEF) $< -o $@

$(OBJECT_DIR)/%.cpp.o: $(OBJECT_DIR)/%.cpp Makefile | $(BUILD_DIRS)
	@echo Compiling $<
	@$(CC) -c $(FLAGS) $(INCLUDES) $(DEF) $< -o $@

.PRECIOUS $(OBJECT_DIR)/%.cpp.o: %.cpp Makefile | $(BUILD_DIRS)
	@echo Compiling $<
	@$(CC) -c $(FLAGS) $(INCLUDES) $(DEF) $< -o $@

$(BIN_DIR)/$(PROJECT_NAME).exe: $(OBJECTS)
	@echo Linking object...
	@$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	@echo Done

clean:
	@rm -fR $(BUILD_DIR)

rebuild:
	@make --no-print-directory clean
	@make --no-print-directory -j
