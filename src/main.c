#include "include/main.h"

// Function to load modules for HDD access
static void LoadModules()
{
    SifExecModuleBuffer(ps2dev9_irx, size_ps2dev9_irx, 0, NULL, NULL);
    SifExecModuleBuffer(iomanx_irx, size_iomanx_irx, 0, NULL, NULL);
    SifExecModuleBuffer(filexio_irx, size_filexio_irx, 0, NULL, NULL);
    fileXioInit();
    SifExecModuleBuffer(ps2atad_irx, size_ps2atad_irx, 0, NULL, NULL);
    SifExecModuleBuffer(ps2hdd_irx, size_ps2hdd_irx, 0, NULL, NULL);
    SifExecModuleBuffer(ps2fs_irx, size_ps2fs_irx, 0, NULL, NULL);
}

// Funtion for returning to OSD when error is encountered
static inline void BootError(void)
{
    SifExitRpc();
    char *args[2] = {"BootError", NULL};
    ExecOSD(1, args);
}

// Function to find .cfg file
static int FindCfgFile(char *cfgFile, size_t cfgFileSize)
{
    DIR *d;
    struct dirent *dir;
    d = opendir("pfs0:/");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char *dot = strrchr(dir->d_name, '.');
            if (dot && (strcasecmp(dot, ".cfg") == 0)) {
                strncpy(cfgFile, dir->d_name, cfgFileSize - 1);
                cfgFile[cfgFileSize - 1] = '\0'; // Ensure null-termination
                closedir(d);
                return 0; // Success
            }
        }
        closedir(d);
    }
    return -1; // Failure
}

// Function to parse the cfgFileName and populate boot_argv
static void ParseFileName(char *cfgFileName, char *boot_argv[])
{
    char *dotPos, *dashPos, *extPos;

    // Find the position of the last "-" in the filename (before CD/DVD)
    dashPos = strrchr(cfgFileName, '-');
    if (dashPos) {
        *dashPos = '\0'; // Terminate the string at the '-' to separate ISO name from media type
        boot_argv[2] = dashPos + 1; // Extract the media type (e.g., CD/DVD)

        // Remove ".cfg" from boot_argv[2] if present
        extPos = strstr(boot_argv[2], ".cfg");
        if (extPos) {
            *extPos = '\0'; // Remove the ".cfg" extension
        }
    } else {
        BootError();
    }

    // Find the second dot in the filename to set boot_argv[0] (ISO name starting after second dot)
    dotPos = strchr(cfgFileName, '.');
    if (dotPos) {
        dotPos = strchr(dotPos + 1, '.'); // Find the second dot
        if (dotPos) {
            boot_argv[0] = dotPos + 1; // Start boot_argv[0] from the character after the second dot
        } else {
            BootError(); // Handle cases with fewer than two dots
        }
    } else {
        BootError(); // Handle cases with fewer than one dot
    }

    // Find the game ID and set it in boot_argv[1]
    if (dotPos) {
        int idLength = dotPos - cfgFileName;
        // Allocate and copy the game ID into boot_argv[1]
        boot_argv[1] = malloc(idLength + 1);
        strncpy(boot_argv[1], cfgFileName, idLength);
        boot_argv[1][idLength] = '\0'; // Null-terminate the game ID
    }
}


// Locate OPL partition if on HDD
void GetOPLPath(char *name, size_t nameSize, char *oplPartition, size_t oplPartitionSize, char *oplFilePath, size_t oplFilePathSize)
{
    int result;
    const char *prefix = "pfs0:";

    // Mount the __common partition
    result = fileXioMount("pfs0:", "hdd0:__common", FIO_MT_RDWR);
    if (result == 0) {
        // Open the configuration file
        FILE *fd = fopen("pfs0:OPL/conf_hdd.cfg", "rb");
        if (fd != NULL) {
            char line[128];
            if (fgets(line, sizeof(line), fd) != NULL) {
                char *val;
                // Find '=' character and get the value part
                if ((val = strchr(line, '=')) != NULL)
                    val++;

                // Copy the value to the name
                snprintf(name, nameSize, "%s", val);

                // Remove Windows-style CR+LF (carriage return and line feed)
                if ((val = strchr(name, '\r')) != NULL)
                    *val = '\0';
                if ((val = strchr(name, '\n')) != NULL)
                    *val = '\0';
            } else {
                // If reading the line fails, use default name
                snprintf(name, nameSize, "+OPL");
            }
            // Close the file after reading
            fclose(fd);
        } else {
            // If file opening fails, use default name
            snprintf(name, nameSize, "+OPL");
        }

        // Unmount the HDD partition
        fileXioUmount("pfs0:");
    } else {
        // If mounting fails, use default name
        snprintf(name, nameSize, "+OPL");
    }

    // Construct oplPartition and oplFilePath
    snprintf(oplPartition, oplPartitionSize, "hdd0:%s", name);
    if (oplPartition[5] != '+') {
        prefix = "pfs0:OPL/";
    }
    snprintf(oplFilePath, oplFilePathSize, "%sOPNPS2LD.ELF", prefix);

    // Mount the OPL partition
    result = fileXioMount("pfs0:", oplPartition, FIO_MT_RDWR);
    if (result != 0) {
        // If mounting fails, call BootError
        BootError();
    }
}

int main(int argc, char *argv[])
{
    char cwd[256];
    char cfgFile[256];

    // Initialize and configure enviroment
    if (argc > 1) {
        while (!SifIopReset(NULL, 0)) {}
        while (!SifIopSync()) {}
        SifInitRpc(0);
    }

    SifInitIopHeap();
    SifLoadFileInit();
    sbv_patch_enable_lmb();
    LoadModules();
    SifLoadFileExit();
    SifExitIopHeap();

    #define NAME_SIZE 128
    #define OPL_PARTITION_SIZE 128
    #define OPL_FILE_PATH_SIZE 256

    char name[NAME_SIZE];
    char oplPartition[OPL_PARTITION_SIZE];
    char oplFilePath[OPL_FILE_PATH_SIZE];

    // Get the current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        BootError();
        return 1;
    }

     // Remove ":pfs:" from the end of the current working directory path
    char *pfs_pos = strstr(cwd, ":pfs:");
    if (pfs_pos) {
        *pfs_pos = '\0'; // Terminate the string before ":pfs:"
    }

    // Mount the current working directory
    if (fileXioMount("pfs0:", cwd, FIO_MT_RDWR) != 0) {
        BootError();
        return 1;
    }

    // Find an cfg file in the current working directory
    if (FindCfgFile(cfgFile, sizeof(cfgFile)) != 0) {
        BootError();
        return 1;
    }

    // Parse the cfg filename and populate boot_argv
    char *boot_argv[4];
    ParseFileName(cfgFile, boot_argv);  // This will populate boot_argv[0], [1], [2]

    // Set the fourth argument (e.g., "bdm")
    boot_argv[3] = "bdm";

    // Unmount cwd
    fileXioUmount("pfs0:");

    GetOPLPath(name, NAME_SIZE, oplPartition, OPL_PARTITION_SIZE, oplFilePath, OPL_FILE_PATH_SIZE);
    
    // Launch OPL with arguments
    if (LoadELFFromFile(oplFilePath, 4, boot_argv) != 0) {
        BootError();
        return 1;
    }

    return 0;
}
