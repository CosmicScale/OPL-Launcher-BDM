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

// Funtion load OPL from default location when error is encountered
static inline void BootError(void)
{
    fileXioUmount("pfs0:");
    fileXioMount ("pfs0:", "hdd0:+OPL", FIO_MT_RDWR);
    LoadELFFromFile("pfs0:/OPNPS2LD.ELF", 0, NULL);
}

// Read cfg file to get info about game to be launched
void GetGameInfo(char *boot_argv[])
{
    char cwd[256];
    // Get the current working directory
    if (getcwd(cwd, 256) == NULL) {
        BootError();
    }

    // Remove ":pfs:" from the end of the current working directory path
    char *pfs_pos = strstr(cwd, ":pfs:");
    if (pfs_pos) {
        *pfs_pos = '\0'; // Terminate the string before ":pfs:"
    }

    // Initialize boot_argv to default values
    for (int i = 0; i < 3; i++) {
        boot_argv[i] = NULL;
    }

    // Mount the current working directory
    int result = fileXioMount("pfs0:", cwd, FIO_MT_RDWR);
    if (result == 0) {
        // Open the configuration file
        FILE *fd = fopen("pfs0:launcher.cfg", "rb");
        if (fd != NULL) {
            char line[128];
            int lineCount = 0;

            // Read lines and extract values
            while (fgets(line, sizeof(line), fd) != NULL && lineCount < 3) {
                char *val = strchr(line, '=');
                if (val) {
                    val++; // Move past '='

                    // Remove Windows-style CR+LF (carriage return and line feed)
                    char *end;
                    if ((end = strchr(val, '\r')) != NULL)
                        *end = '\0';
                    if ((end = strchr(val, '\n')) != NULL)
                        *end = '\0';

                    // Allocate memory for the value and store it in boot_argv
                    boot_argv[lineCount] = strdup(val);
                }

                lineCount++;
            }

            // Close the file after reading
            fclose(fd);
        } else {
            BootError();
        }

        // Unmount the HDD partition
        fileXioUmount("pfs0:");
    } else {
        BootError();
    }
}

// Locate OPL partition on HDD
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
    // init_scr();

    // Parse the cfg filename and populate boot_argv
    char *boot_argv[4];
    GetGameInfo(boot_argv);

    if (strcmp(boot_argv[2], "POPS") == 0) {
        // Do this for "POPS"
        // Replace the file extension of boot_argv[0] with .ELF
        char *ext = strrchr(boot_argv[0], '.'); // Find the last occurrence of '.'
        if (ext != NULL) {
            strcpy(ext, ".ELF"); // Replace the extension with .ELF
        } else {
            // If no extension is found, append .ELF
            strcat(boot_argv[0], ".ELF");
        }

        // Mount the __.POPS directory
        if (fileXioMount("pfs0:", "hdd0:__.POPS", FIO_MT_RDWR) != 0) {
            BootError();
            return 1;
        }

        // Load the found ELF file
        char fullFilePath[512];
        snprintf(fullFilePath, sizeof(fullFilePath), "pfs0:/%s", boot_argv[0]);
        if (LoadELFFromFile(fullFilePath, 0, NULL) != 0) {
            BootError();
            return 1;
        }

    } else {
        // Set the fourth argument
        boot_argv[3] = "bdm";

        #define NAME_SIZE 128
        #define OPL_PARTITION_SIZE 128
        #define OPL_FILE_PATH_SIZE 256

        char name[NAME_SIZE];
        char oplPartition[OPL_PARTITION_SIZE];
        char oplFilePath[OPL_FILE_PATH_SIZE];

        GetOPLPath(name, NAME_SIZE, oplPartition, OPL_PARTITION_SIZE, oplFilePath, OPL_FILE_PATH_SIZE);
    
        // Launch OPL with arguments
        if (LoadELFFromFile(oplFilePath, 4, boot_argv) != 0) {
            BootError();
         return 1;
        }
    }

     return 0;
}