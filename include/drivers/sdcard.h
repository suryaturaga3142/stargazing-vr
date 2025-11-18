#ifndef SDCARD_H
#define SDCARD_H

#include "ff.h" // FatFs library header

void cd(int argc, char *argv[]);
void input(int argc, char *argv[]);
void ls(int argc, char *argv[]);
void mkdir(int argc, char *argv[]);
void mount(int argc, char *argv[]);
void pwd(int argc, char *argv[]);
void rm(int argc, char *argv[]);
void cat(int argc, char *argv[]);
void append(int argc, char *argv[]);
void date(int argc, char *argv[]);
void restart(int argc, char *argv[]);

#endif