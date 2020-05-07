#not need to set PROJECT_HOME in sub folder
export PROJECT_HOME:=$(shell pwd|sed "s/\/$$//g;")
export STD:=c++14
#The example setting
#TYPE:=EXE/STATIC/SHARE/REF/NONE
#STD:=c++17
#VERSION:=0.0.1
#DEPS:=library/test_lib library/core
#INC_PATHS:=/usr/include/openssl /user/include/boost
#LIB_PATHS:=/usr/lib64 /usr/local/lib64
#LIBS:=boost_system ssl
#C_FLAGS:=-O2
#CPP_FLAGS:=-O2
#EXE_FLAGS:=-pthread
#SHARE_FLAGS:=-pthread
#The example setting end
include $(PROJECT_HOME)/common.mk