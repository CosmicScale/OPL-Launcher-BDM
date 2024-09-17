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

// Function to extract the filename from a full path
static char *GetFileName(const char *filePath)
{
    scr_printf("File Path: %s\n", filePath);
    // Check if the file path starts with "mass:" and skip it if so
    const char *prefix = "mass:";
    if (strncmp(filePath, prefix, strlen(prefix)) == 0) {
        filePath += strlen(prefix); // Skip "mass:" prefix
    }

    // Find the last occurrence of '/' in the path
    char *fileName = strrchr(filePath, '/');
    if (fileName)
        return fileName + 1; // Return the file name part (skip the '/')
    return (char *)filePath; // If no '/', return the original path (likely just the file name)
}

// Function to parse the elfFileName and populate boot_argv
static void ParseFileName(char *elfFileName, char *boot_argv[])
{
    char *dotPos, *dashPos, *extPos;

    // Find the position of the last "-" in the filename (before CD/DVD)
    dashPos = strrchr(elfFileName, '-');
    if (dashPos) {
        *dashPos = '\0'; // Terminate the string at the '-' to separate ISO name from media type
        boot_argv[2] = dashPos + 1; // Extract the media type (e.g., CD/DVD)

        // Remove ".elf" from boot_argv[2] if present
        extPos = strstr(boot_argv[2], ".elf");
        if (extPos) {
            *extPos = '\0'; // Remove the ".elf" extension
        }
    } else {
        BootError();
    }

    // Set boot_argv[0] to the full ISO filename (before the dash)
    boot_argv[0] = elfFileName;

    // Find the second dot in the filename to extract the game ID (e.g., "SLUS_203.75")
    dotPos = strchr(elfFileName, '.');
    if (dotPos) {
        dotPos = strchr(dotPos + 1, '.'); // Find the second dot
        if (dotPos) {
            int idLength = dotPos - elfFileName;
            // Allocate and copy the game ID into boot_argv[1]
            boot_argv[1] = malloc(idLength + 1);
            strncpy(boot_argv[1], elfFileName, idLength);
            boot_argv[1][idLength] = '\0'; // Null-terminate the game ID
        }
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
    init_scr();

    #define NAME_SIZE 128
    #define OPL_PARTITION_SIZE 128
    #define OPL_FILE_PATH_SIZE 256

    char name[NAME_SIZE];
    char oplPartition[OPL_PARTITION_SIZE];
    char oplFilePath[OPL_FILE_PATH_SIZE];

    GetOPLPath(name, NAME_SIZE, oplPartition, OPL_PARTITION_SIZE, oplFilePath, OPL_FILE_PATH_SIZE);

    scr_printf("OPL Partition: %s\n", oplPartition);
    sleep(2);
    scr_printf("OPL File Path: %s\n", oplFilePath);
    sleep(2);

    // Get the filename of the executable from argv[0]
    char *elfFileName = GetFileName(argv[0]);
    scr_printf("ELF file name: %s\n", elfFileName);  // Print the ELF file name for debugging
    sleep(2);

    // Parse the ELF filename and populate boot_argv
    char *boot_argv[4];
    ParseFileName(elfFileName, boot_argv);  // This will populate boot_argv[0], [1], [2]

    // Set the fourth argument (e.g., "bdm")
    boot_argv[3] = "bdm";

    // Print boot_argv values for debugging
    scr_printf("boot_argv[0]: %s\n", boot_argv[0]);
    sleep(2);
    scr_printf("boot_argv[1]: %s\n", boot_argv[1]);
    sleep(2);
    scr_printf("boot_argv[2]: %s\n", boot_argv[2]);
    sleep(2);
    scr_printf("boot_argv[3]: %s\n", boot_argv[3]);
    sleep(2);
    
    // Launch OPL with arguments
    scr_printf("Launching OPL...");
    sleep(5);
    if (LoadELFFromFile(oplFilePath, 4, boot_argv) != 0) {
        BootError();
        return 1;
    }

    return 0;
}
