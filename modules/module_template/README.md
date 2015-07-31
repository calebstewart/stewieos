# Kernel Module Template

This template can be used to quickly start up a new kernel module. The only modifications needed are as follows,

1. Copy this template directory and rename it with the lower case name of your project with underscores instead of spaces.
2. Change the PROJECT_NAME variable in the Makefile to the name of your project/directory.
3. Replace all occurances of "module_template" in main.c with the project name/directory name.
4. Overwrite this readme with one relevant to your module.
5. Add your module to the list of modules within StewieOS/modules/Makefile by appending your project name to the PROJECTS variable (this should match the directory name).

After that, your module should be automatically compiled and installed with the kernel. You are responsible for loading the module at boot time, which can be done through /etc/modules.conf or an existing user space application.
