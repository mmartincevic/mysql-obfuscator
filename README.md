# mysql-obfuscator
Mysql plugin to obfuscate SQL Queries and thus protect data returned. 

This is no where near, for now, to working and production useable plugin but at least it can give you an idea on where to start
writing plugin, where you can modify it and extend it towards your needs.


Few things noticed:

Compiler will create plugin inside plugin directory, please create subdirectory called "obfuscator" in plugin directory and then run

cmake --build .  (you can, of course pass other options)

This will build whole project but also it will create library obfuscator in plugin/obfuscator directory. 
You can then copy the plugin compiled to MariaDB plugin directory for loading them

If you're unsure go to mysql console and type:

show variables like 'plugin_dir';  <- this will point you to your directory


After putting obfuscator to correct directory (of course you can always create a symlink as well)
Restart server (maybe not needed but just tbs)

INSTALL PLUGIN obfuscator SONAME 'libobfuscator.dll';

You will notice that in mysql database obfuscator_rules table will be created. No worries because upon calling UNINSTALL PLUGIN obfuscator;
that table will be deleted.
