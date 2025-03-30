#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h> // For access, mkdir
#include <errno.h>  // For errno
#include <libgen.h> // For dirname

#define MAX_FILES 500
#define MAX_PATH_LENGTH 1024
#define OUTPUT_FILE "/home/mert/AndroidStudioProjects/antinsfw_copy/custom-codebase.md" // Kept for now, but should ideally use a save dialog
#define CONFIG_DIR_SUFFIX ".config/codebase-exporter"
#define CONFIG_FILE_NAME "last_dir.txt"

typedef struct {
    char filename[256];
    char path[MAX_PATH_LENGTH];
    GtkWidget *checkbox;
    gboolean selected;
} FileEntry;

typedef struct {
    const char *name;
    int extension_count;
    const char **extensions;
} ProjectType;

// Define project types with their associated file extensions
const char *android_extensions[] = {"java", "kt", "kts", "xml", "gradle"};
const char *web_extensions[] = {"html", "css", "js", "jsx", "ts", "tsx", "json"};
const char *python_extensions[] = {"py", "pyw", "pyx", "pyd"};
const char *cpp_extensions[] = {"c", "cpp", "h", "hpp", "cc"};
const char *golang_extensions[] = {"go"};
const char *rust_extensions[] = {"rs"};

ProjectType project_types[] = {
    {"Android", 5, android_extensions},
    {"Web/Node.js", 7, web_extensions},
    {"Python", 4, python_extensions},
    {"C/C++", 5, cpp_extensions},
    {"Go", 1, golang_extensions},
    {"Rust", 1, rust_extensions},
    {NULL, 0, NULL} // Sentinel to mark the end
};

FileEntry files[MAX_FILES];
int file_count = 0;
GtkWidget *project_path_entry;
GtkWidget *project_type_combo;
GtkWidget *status_label;
GtkClipboard *clipboard;

// --- Configuration File Handling ---

// Ensure the configuration directory exists
int ensure_config_dir_exists() {
    char config_path[MAX_PATH_LENGTH];
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        fprintf(stderr, "Error: HOME environment variable not set.\n");
        return -1;
    }
    snprintf(config_path, MAX_PATH_LENGTH, "%s/%s", home_dir, CONFIG_DIR_SUFFIX);

    struct stat st = {0};
    if (stat(config_path, &st) == -1) {
        // Attempt to create directory recursively (mkdir -p equivalent)
        char *path_copy = strdup(config_path);
        if (!path_copy) {
            perror("strdup failed");
            return -1;
        }
        char *p = path_copy;
        // Skip leading '/' if any
        if (*p == '/') p++;
        while ((p = strchr(p, '/'))) {
            *p = '\0';
            if (mkdir(path_copy, 0700) == -1) {
                if (errno != EEXIST) {
                    perror("mkdir failed");
                    free(path_copy);
                    return -1;
                }
            }
            *p = '/';
            p++;
        }
        // Create the final directory
        if (mkdir(path_copy, 0700) == -1) {
            if (errno != EEXIST) {
                perror("mkdir final failed");
                free(path_copy);
                return -1;
            }
        }
        free(path_copy);
        printf("Created config directory: %s\n", config_path);
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: %s exists but is not a directory.\n", config_path);
        return -1;
    }
    return 0;
}

// Get the full path to the config file
char* get_config_file_path() {
    static char config_file_path[MAX_PATH_LENGTH]; // Static buffer
    const char *home_dir = getenv("HOME");
    if (!home_dir) return NULL;
    snprintf(config_file_path, MAX_PATH_LENGTH, "%s/%s/%s", home_dir, CONFIG_DIR_SUFFIX, CONFIG_FILE_NAME);
    return config_file_path;
}

// Read the last directory from the config file
char* read_last_directory() {
    char *config_file = get_config_file_path();
    if (!config_file) return NULL;

    FILE *file = fopen(config_file, "r");
    if (!file) {
        //perror("Could not open config file for reading"); // Don't print error if file just doesn't exist
        return NULL;
    }

    char *path = (char*)malloc(MAX_PATH_LENGTH);
    if (!path) {
        fclose(file);
        perror("malloc failed for reading config");
        return NULL;
    }

    if (fgets(path, MAX_PATH_LENGTH, file) != NULL) {
        // Remove trailing newline if present
        size_t len = strlen(path);
        if (len > 0 && path[len - 1] == '\n') {
            path[len - 1] = '\0';
        }
        fclose(file);
        // Check if directory still exists
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
             return path;
        } else {
             fprintf(stderr, "Warning: Last directory '%s' from config not found or not a directory.\n", path);
             free(path); // Free memory if path is invalid
        }
    } else {
        free(path); // Free memory if fgets fails or file is empty
    }

    fclose(file);
    return NULL; // Return NULL if read failed, file empty, or path invalid
}

// Write the last directory to the config file
void write_last_directory(const char *path) {
    if (!path) return;

    char *config_file = get_config_file_path();
    if (!config_file) return;

    FILE *file = fopen(config_file, "w");
    if (!file) {
        perror("Could not open config file for writing");
        return;
    }

    fprintf(file, "%s\n", path);
    fclose(file);
}

// Function to read file content
char* read_file_content(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Cannot open file %s\n", path);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    // Allocate memory for the entire file content plus null terminator
    char *buffer = (char*) malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    // Read file content
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';  // Null-terminate the string
    
    fclose(file);
    return buffer;
}

// Function to determine file language extension
const char* get_language_extension(const char *filename) {
    char *dot = strrchr(filename, '.');
    if (dot) {
        return dot + 1;  // Return extension without the dot
    }
    return "";
}

// Function to check if a file extension is in the allowed list
int is_extension_allowed(const char *filename) {
    int project_type_index = gtk_combo_box_get_active(GTK_COMBO_BOX(project_type_combo));
    if (project_type_index < 0) return 1; // If no project type selected, allow all
    
    char *dot = strrchr(filename, '.');
    if (!dot) return 0; // No extension
    
    const char *ext = dot + 1;
    
    for (int i = 0; i < project_types[project_type_index].extension_count; i++) {
        if (strcasecmp(ext, project_types[project_type_index].extensions[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

// Function to generate markdown content
char* generate_markdown_content() {
    // First calculate the required buffer size
    size_t total_size = 0;
    for (int i = 0; i < file_count; i++) {
        if (files[i].selected) {
            char *content = read_file_content(files[i].path);
            if (content) {
                total_size += strlen(content);
                free(content);
            }
            
            // Add size for markdown formatting
            total_size += 1024; // Extra space for formatting
        }
    }
    
    // Allocate buffer - handle potential large size
    if (total_size == 0) total_size = 1; // Allocate at least 1 byte if nothing selected
    char *buffer = (char*)malloc(total_size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for markdown buffer (size: %zu)\n", total_size);
        return NULL;
    }
    
    buffer[0] = '\0';
    
    // Generate markdown content
    for (int i = 0; i < file_count; i++) {
        if (files[i].selected) {
            // Get relative path from project root
            const char *project_path = gtk_entry_get_text(GTK_ENTRY(project_path_entry));
            char relative_path[MAX_PATH_LENGTH];
            
            if (strncmp(files[i].path, project_path, strlen(project_path)) == 0) {
                sprintf(relative_path, "%s", files[i].path + strlen(project_path) + 1); // +1 to skip the slash
            } else {
                strcpy(relative_path, files[i].filename);
            }
            
            // Write file header with relative path
            char header[MAX_PATH_LENGTH + 10];
            sprintf(header, "- %s\n", relative_path);
            strcat(buffer, header);
            
            // Determine language for code block
            const char *lang = get_language_extension(files[i].filename);
            char lang_marker[30];
            sprintf(lang_marker, "```%s\n", lang);
            strcat(buffer, lang_marker);
            
            // Read and write file content
            char *content = read_file_content(files[i].path);
            if (content) {
                strcat(buffer, content);
                free(content);
            } else {
                strcat(buffer, "Error reading file content\n");
            }
            
            // End code block
            strcat(buffer, "\n```\n\n");
        }
    }
    
    return buffer;
}

// Function to save selected files to markdown
void save_to_markdown() {
    char *markdown_content = generate_markdown_content();
    if (!markdown_content) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to generate markdown content");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    FILE *md_file = fopen(OUTPUT_FILE, "w");
    if (!md_file) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to open output file for writing");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        free(markdown_content);
        return;
    }
    
    fprintf(md_file, "%s", markdown_content);
    fclose(md_file);
    
    int selected_count = 0;
    for (int i = 0; i < file_count; i++) {
        if (files[i].selected) selected_count++;
    }
    
    if (selected_count > 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_INFO,
                                                  GTK_BUTTONS_OK,
                                                  "Saved %d files to %s",
                                                  selected_count, OUTPUT_FILE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_WARNING,
                                                  GTK_BUTTONS_OK,
                                                  "No files were selected");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    
    free(markdown_content);
}

// Function to copy to clipboard
void copy_to_clipboard() {
    char *markdown_content = generate_markdown_content();
    if (!markdown_content) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to generate markdown content");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    
    gtk_clipboard_set_text(clipboard, markdown_content, -1);
    free(markdown_content);
    
    int selected_count = 0;
    for (int i = 0; i < file_count; i++) {
        if (files[i].selected) selected_count++;
    }
    
    if (selected_count > 0) {
        char status_text[100];
        sprintf(status_text, "Copied %d files to clipboard", selected_count);
        gtk_label_set_text(GTK_LABEL(status_label), status_text);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_WARNING,
                                                  GTK_BUTTONS_OK,
                                                  "No files were selected");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// Callback for save button
void on_save_clicked(GtkWidget *widget, gpointer data) {
    // Update selected status for all files
    for (int i = 0; i < file_count; i++) {
        files[i].selected = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(files[i].checkbox));
    }
    
    save_to_markdown();
}

// Callback for copy to clipboard button
void on_copy_clicked(GtkWidget *widget, gpointer data) {
    // Update selected status for all files
    for (int i = 0; i < file_count; i++) {
        files[i].selected = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(files[i].checkbox));
    }
    
    copy_to_clipboard();
}

// Callback for select all button
void on_select_all_clicked(GtkWidget *widget, gpointer data) {
    for (int i = 0; i < file_count; i++) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(files[i].checkbox), TRUE);
    }
}

// Callback for clear all button
void on_clear_all_clicked(GtkWidget *widget, gpointer data) {
    for (int i = 0; i < file_count; i++) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(files[i].checkbox), FALSE);
    }
}

// Detect project type from project path
int detect_project_type(const char *project_path) {
    // Check for Android project
    char build_gradle_path[MAX_PATH_LENGTH];
    sprintf(build_gradle_path, "%s/build.gradle", project_path);
    if (access(build_gradle_path, F_OK) == 0) {
        return 0; // Android
    }
    
    // Check for package.json (Node.js/Web)
    char package_json_path[MAX_PATH_LENGTH];
    sprintf(package_json_path, "%s/package.json", project_path);
    if (access(package_json_path, F_OK) == 0) {
        return 1; // Web/Node.js
    }
    
    // Check for requirements.txt (Python)
    char requirements_path[MAX_PATH_LENGTH];
    sprintf(requirements_path, "%s/requirements.txt", project_path);
    if (access(requirements_path, F_OK) == 0) {
        return 2; // Python
    }
    
    // Check for CMakeLists.txt (C/C++)
    char cmake_path[MAX_PATH_LENGTH];
    sprintf(cmake_path, "%s/CMakeLists.txt", project_path);
    if (access(cmake_path, F_OK) == 0) {
        return 3; // C/C++
    }
    
    // Check for go.mod (Go)
    char go_mod_path[MAX_PATH_LENGTH];
    sprintf(go_mod_path, "%s/go.mod", project_path);
    if (access(go_mod_path, F_OK) == 0) {
        return 4; // Go
    }
    
    // Check for Cargo.toml (Rust)
    char cargo_path[MAX_PATH_LENGTH];
    sprintf(cargo_path, "%s/Cargo.toml", project_path);
    if (access(cargo_path, F_OK) == 0) {
        return 5; // Rust
    }
    
    return -1; // Unknown
}

// Callback when a file checkbox is toggled
void on_file_toggled(GtkToggleButton *toggle_button, gpointer user_data) {
    FileEntry *file_info = (FileEntry*)user_data;
    file_info->selected = gtk_toggle_button_get_active(toggle_button);
}

// Function to load files from directory
void load_files_from_directory(const char *dir_path, GtkWidget *files_container) {
    // Clear previous file list
    for (int i = 0; i < file_count; i++) {
        if (files[i].checkbox) { // Only destroy if widget was created
             gtk_widget_destroy(files[i].checkbox);
             files[i].checkbox = NULL; // Avoid double free on refresh
        }
    }
    file_count = 0;

    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    dir = opendir(dir_path);
    if (!dir) {
        char error_msg[200];
        snprintf(error_msg, sizeof(error_msg), "Failed to open directory: %s", dir_path);
        gtk_label_set_text(GTK_LABEL(status_label), error_msg);
        perror("opendir failed");
        return;
    }

    gtk_label_set_text(GTK_LABEL(status_label), "Loading files...");

    char current_path[MAX_PATH_LENGTH];
    strncpy(current_path, dir_path, MAX_PATH_LENGTH -1);
    current_path[MAX_PATH_LENGTH - 1] = '\0'; // Ensure null termination

    GList *subdirs = NULL; // List to hold subdirectories for recursive calls

    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        // Skip hidden files/directories and ., ..
        if (entry->d_name[0] == '.') continue;

        // Use g_build_filename for safer path construction
        char *full_path = g_build_filename(current_path, entry->d_name, NULL);
        if (!full_path) {
             perror("g_build_filename failed");
             continue; // Skip this entry
        }

        if (stat(full_path, &file_stat) == -1) {
            perror("stat failed");
            g_free(full_path);
            continue; // Skip this entry
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Add subdirectory path to the list for later processing
            subdirs = g_list_prepend(subdirs, full_path); // Prepend transfers ownership
        } else if (S_ISREG(file_stat.st_mode)) {
            if (is_extension_allowed(entry->d_name)) {
                strncpy(files[file_count].filename, entry->d_name, 255);
                files[file_count].filename[255] = '\0';
                strncpy(files[file_count].path, full_path, MAX_PATH_LENGTH - 1);
                files[file_count].path[MAX_PATH_LENGTH - 1] = '\0';

                // Create checkbox for the file
                files[file_count].checkbox = gtk_check_button_new_with_label(entry->d_name);
                files[file_count].selected = TRUE; // Default to selected
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(files[file_count].checkbox), TRUE);

                // Connect toggle signal
                g_signal_connect(files[file_count].checkbox, "toggled", G_CALLBACK(on_file_toggled), &files[file_count]);

                gtk_box_pack_start(GTK_BOX(files_container), files[file_count].checkbox, FALSE, FALSE, 0);
                gtk_widget_show(files[file_count].checkbox);

                file_count++;
            }
             g_free(full_path); // Free path if it was a file
        } else {
             g_free(full_path); // Free path if neither dir nor file
        }
    }
    closedir(dir);

    // Process subdirectories recursively
    GList *iterator = NULL;
    for(iterator = subdirs; iterator; iterator = g_list_next(iterator)) {
        if (file_count >= MAX_FILES) {
            fprintf(stderr, "Warning: Maximum file limit (%d) reached. Some files might be skipped.\n", MAX_FILES);
            g_free(iterator->data); // Free remaining paths in the list
            continue;
        }
        load_files_from_directory((char*)iterator->data, files_container);
        g_free(iterator->data); // Free the path string after processing
    }
    g_list_free(subdirs); // Free the list structure itself

    if (file_count == 0) {
         gtk_label_set_text(GTK_LABEL(status_label), "No allowed files found in the selected directory.");
    } else {
        char status_msg[100];
        snprintf(status_msg, sizeof(status_msg), "Loaded %d files. Ready.", file_count);
        gtk_label_set_text(GTK_LABEL(status_label), status_msg);
    }
}

// Callback for project type combo box
void on_project_type_changed(GtkWidget *widget, gpointer data) {
    // Reload files with new filter
    const char *dir_path = gtk_entry_get_text(GTK_ENTRY(project_path_entry));
    load_files_from_directory(dir_path, GTK_WIDGET(data));
}

// Callback for browse button
void on_browse_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
    gint res;
    
    dialog = gtk_file_chooser_dialog_new("Select Project Directory",
                                        NULL,
                                        action,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "_Open",
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
    
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        char *folder_path = gtk_file_chooser_get_filename(chooser);
        
        // Update path entry
        gtk_entry_set_text(GTK_ENTRY(project_path_entry), folder_path);
        
        // Reload files
        load_files_from_directory(folder_path, data); // data is files_container

        // Save the selected directory to config
        write_last_directory(folder_path);

        g_free(folder_path);
    }
    
    gtk_widget_destroy(dialog);
}

// Callback for refresh button
void on_refresh_clicked(GtkWidget *widget, gpointer data) {
    const char *dir_path = gtk_entry_get_text(GTK_ENTRY(project_path_entry));
    load_files_from_directory(dir_path, GTK_WIDGET(data));
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *path_box;
    GtkWidget *path_label;
    GtkWidget *browse_button;
    GtkWidget *refresh_button;
    GtkWidget *type_box;
    GtkWidget *type_label;
    GtkWidget *scrolled_window;
    GtkWidget *files_vbox;
    GtkWidget *button_box;
    GtkWidget *select_all_button;
    GtkWidget *clear_all_button;
    GtkWidget *save_button;
    GtkWidget *copy_button;
    
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Get default clipboard
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    
    // --- Read last directory from config ---
    if (ensure_config_dir_exists() != 0) {
        fprintf(stderr, "Warning: Could not ensure config directory exists.\n");
    }
    char *last_dir = read_last_directory();

    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "File Selector for Codebase Export");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Create main container
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), main_box);
    
    // Path selection
    path_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    path_label = gtk_label_new("Project Path:");
    project_path_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(project_path_entry), last_dir ? last_dir : ""); // Set from config or empty
    browse_button = gtk_button_new_with_label("Browse");
    refresh_button = gtk_button_new_with_label("Refresh");
    
    gtk_box_pack_start(GTK_BOX(path_box), path_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(path_box), project_path_entry, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(path_box), browse_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(path_box), refresh_button, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(main_box), path_box, FALSE, FALSE, 5);
    
    // Project type selection
    type_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    type_label = gtk_label_new("Project Type:");
    project_type_combo = gtk_combo_box_text_new();
    
    // Add project types
    for (int i = 0; project_types[i].name != NULL; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(project_type_combo), project_types[i].name);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(project_type_combo), 0); // Default to Android
    
    gtk_box_pack_start(GTK_BOX(type_box), type_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(type_box), project_type_combo, FALSE, FALSE, 5);
    
    // Status label
    status_label = gtk_label_new("Ready");
    gtk_box_pack_end(GTK_BOX(type_box), status_label, TRUE, TRUE, 5);
    
    gtk_box_pack_start(GTK_BOX(main_box), type_box, FALSE, FALSE, 5);
    
    // Files list
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled_window, TRUE, TRUE, 5);
    
    files_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scrolled_window), files_vbox);
    
    // Button box
    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    select_all_button = gtk_button_new_with_label("Select All");
    clear_all_button = gtk_button_new_with_label("Clear All");
    save_button = gtk_button_new_with_label("Save to Markdown");
    copy_button = gtk_button_new_with_label("Copy to Clipboard");
    
    gtk_box_pack_end(GTK_BOX(button_box), save_button, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(button_box), copy_button, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(button_box), clear_all_button, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(button_box), select_all_button, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 5);
    
    // Connect signals
    g_signal_connect(browse_button, "clicked", G_CALLBACK(on_browse_clicked), files_vbox);
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_clicked), files_vbox);
    g_signal_connect(project_type_combo, "changed", G_CALLBACK(on_project_type_changed), files_vbox);
    g_signal_connect(select_all_button, "clicked", G_CALLBACK(on_select_all_clicked), NULL);
    g_signal_connect(clear_all_button, "clicked", G_CALLBACK(on_clear_all_clicked), NULL);
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked), NULL);
    g_signal_connect(copy_button, "clicked", G_CALLBACK(on_copy_clicked), NULL);
    
    // Initial load of files from the last directory
    if (last_dir) {
        // Detect and set project type for the last directory
        int detected_type = detect_project_type(last_dir);
         if (detected_type != -1) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(project_type_combo), detected_type);
         }
        load_files_from_directory(last_dir, files_vbox);
        free(last_dir); // Free the memory allocated by read_last_directory
    } else {
         // Optional: Set a default message if no last directory
         gtk_label_set_text(GTK_LABEL(status_label), "Browse to select a project directory.");
    }

    // Show all widgets
    gtk_widget_show_all(window);
    
    // Start GTK main loop
    gtk_main();
    
    return 0;
}
