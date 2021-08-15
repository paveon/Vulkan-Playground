#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Application.h"

#include <gtk/gtk.h>
#include <GLFW/glfw3.h>

//#define GLFW_EXPOSE_NATIVE_X11
//#include <GLFW/glfw3native.h>


auto FileDialogs::OpenFile(const char* filterName, const char *filter) -> std::string {
    GtkFileFilter *gtkFilter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(gtkFilter, filter);
    gtk_file_filter_set_name(gtkFilter, filterName);

    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",
                                                    nullptr,
                                                    action,
                                                    ("_Cancel"),
                                                    GTK_RESPONSE_CANCEL,
                                                    ("_Open"),
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), gtkFilter);
    std::string filepath;
    switch (gtk_dialog_run(GTK_DIALOG (dialog))) {
        case GTK_RESPONSE_ACCEPT: {
            char *tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            filepath = std::string(tmp);
            g_free(tmp);
            break;
        }
    }
    gtk_widget_destroy(dialog);
//    gtk_window_close(GTK_WINDOW(dialog));
    while (gtk_events_pending()) {
        gtk_main_iteration_do(false);
    }
    return filepath;
}

auto FileDialogs::SaveFile(const char* filterName, const char *filter) -> std::string {
    return "";
}