- When you type `/opt/homebrew/Cellar/gdal/3.11.0_2/lib/gdalplugins`, your shell tries to execute it as a program, which results in a "permission denied" error because directories cannot be executed.
- Using `sudo` does not help, as the directory still cannot be executed, leading to "command not found."

---
How to correctly find the GDAL installation directory on Apple Silicon with Homebrew
## Check if the GDAL directory exists:
Run:
```text
ls /opt/homebrew/Cellar/gdal/

```
This will list all installed GDAL versions under Homebrew’s Cellar directory. The directory name should exactly match the version string returned by gdal-config --version, including any suffix like _2.

## What You Should Do

**To view the contents of the directory, use the `ls` command:**
```sh
ls /opt/homebrew/Cellar/gdal/3.11.0_2/lib/gdalplugins
```
This will list files (such as your `test_plugin` and `drivers.ini`) inside the directory.

**To copy your plugin into this directory:**
```sh
cp /path/to/your/test_plugin.dylib /opt/homebrew/Cellar/gdal/3.11.0_2/lib/gdalplugins/
```
You may need `sudo` if you lack write permissions:
```sh
sudo cp /path/to/your/test_plugin.dylib /opt/homebrew/Cellar/gdal/3.11.0_2/lib/gdalplugins/
```

---

## Summary Table

| Command Example                                                                 | Purpose                        |
|---------------------------------------------------------------------------------|--------------------------------|
| `ls /opt/homebrew/Cellar/gdal/3.11.0_2/lib/gdalplugins`                         | List contents of plugin dir    |
| `cp /path/to/test_plugin.dylib /opt/homebrew/Cellar/gdal/3.11.0_2/lib/gdalplugins/` | Copy plugin into directory     |
| `sudo cp ...`                                                                   | Copy with elevated permissions |

---

**Remember:**  
Directories cannot be executed. Always use `ls`, `cp`, `mv`, or similar commands to interact with directories and their contents.