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

// Function to calculate CRC
static uint8_t gameid_calc_crc(const uint8_t *data, int len) {
    uint8_t crc = 0x00;
    for (int i = 0; i < len; i++) {
        crc += data[i];
    }
    return 0x100 - crc;
}

// Function to draw game ID using GSKit
void gameid_draw(GSGLOBAL *gsGlobal, int screenWidth, int screenHeight, const char* game_id) {
    uint8_t data[64] = { 0 };

    int dpos = 0;
    data[dpos++] = 0xA5;  // detect word
    data[dpos++] = 0x00;  // address offset
    dpos++;
    data[dpos++] = strlen(game_id);  // Length of game_id

    memcpy(&data[dpos], game_id, strlen(game_id));
    dpos += strlen(game_id);

    data[dpos++] = 0x00;
    data[dpos++] = 0xD5;  // end word
    data[dpos++] = 0x00;  // padding

    int data_len = dpos;
    data[2] = gameid_calc_crc(&data[3], data_len - 3);

    int xstart = (screenWidth / 2) - (data_len * 8);
    int ystart = screenHeight - (((screenHeight / 8) * 2) + 20);
    int height = 2;

    for (int i = 0; i < data_len; i++) {
        for (int ii = 7; ii >= 0; ii--) {
            int x = xstart + (i * 16 + ((7 - ii) * 2));
            int x1 = x + 1;

            gsKit_prim_sprite(gsGlobal, x, ystart, x1, ystart + height, 1, GS_SETREG_RGBA(0xFF, 0x00, 0xFF, 0x80));

            uint32_t color = (data[i] >> ii) & 1 ? GS_SETREG_RGBA(0x00, 0xFF, 0xFF, 0x80) : GS_SETREG_RGBA(0xFF, 0xFF, 0x00, 0x80);
            gsKit_prim_sprite(gsGlobal, x1, ystart, x1 + 1, ystart + height, 1, color);
        }
    }
}

// Function to handle GSKit initialization and game ID display
void DisplayGameID(const char *game_id)
{
    // Initialize GSKit
    GSGLOBAL *gsGlobal = gsKit_init_global();

    // Set the screen mode
    gsGlobal->Mode = GS_MODE_DTV_480P;  // Set to 480p mode
    gsGlobal->Width = 640;  // Width of the screen
    gsGlobal->Height = 448;  // Height of the screen
    gsGlobal->Interlace = GS_NONINTERLACED;  // Non-interlaced mode for progressive scan
    gsGlobal->Field = GS_FRAME;  // Frame mode
    gsGlobal->PSM = GS_PSM_CT24;  // Pixel storage format (24-bit color)
    gsGlobal->PSMZ = GS_PSMZ_32;  // Z buffer format (32-bit depth)
    gsGlobal->DoubleBuffering = GS_SETTING_OFF;  // Disable double buffering
    gsGlobal->ZBuffering = GS_SETTING_OFF;  // Disable Z buffering

    // Initialize the screen
    gsKit_vram_clear(gsGlobal);  // Clear VRAM
    gsKit_init_screen(gsGlobal);

    // Display game ID
    gameid_draw(gsGlobal, gsGlobal->Width, gsGlobal->Height, game_id);

    // Sync and flip the Graphics Synthesizer
    gsKit_sync_flip(gsGlobal);
    gsKit_queue_exec(gsGlobal);
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

        // Display the game ID on the screen
        DisplayGameID(boot_argv[1]);

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
        
        // Display the game ID on the screen
        DisplayGameID(boot_argv[1]);

        // Launch OPL with arguments
        if (LoadELFFromFile(oplFilePath, 4, boot_argv) != 0) {
            BootError();
         return 1;
        }
    }

     return 0;
}