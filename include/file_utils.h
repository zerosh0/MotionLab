#ifndef FILE_UTILS_H
#define FILE_UTILS_H

char* sfd_open_file(const char* filter);
char* sfd_save_file(const char* filter, const char* defaultExt, const char* initialName);

#endif