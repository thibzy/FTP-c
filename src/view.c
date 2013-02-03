#include <glib/gerror.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <stdio.h>
#include "client.c"
#include <glib.h>
#include <string.h>


/** HIERARCHIE **/
/*
window
box
    boxConnect
        editAddress
        editSocket
        buttonConnect
    paned
        -scrolledWindowConsole
            textViewConsole
        -panedTreeView
            -boxLocal
                labelLocal
                scrolledWindowLocal
                    treeviewLocal
            -boxServer
                labelServer
                scrolledWindowServer
                    treeviewServer
*/

/** VARIABLES GLOBALES **/
    //Elements
    GtkWidget *win = NULL;
    GtkWidget *box = NULL;
    GtkWidget *paned = NULL;
    GtkWidget *statusbar;

    //Connexion
    GtkWidget *boxConnect = NULL;
    GtkWidget *editAddress = NULL, *editSocket = NULL;
    GtkWidget *buttonConnect = NULL;

    //Console
    GtkWidget *scrolledWindowConsole;
    GtkWidget *textViewConsole = NULL;
    GtkTextBuffer *textBufferConsole = NULL;
    GtkTextIter iter;

    //Dossiers
    GtkWidget *boxLocal, *boxServer;
    GtkWidget *labelLocal, *labelServer;
    GtkWidget *scrolledWindowLocal, *scrolledWindowServer;
    GtkWidget *panedTreeView = NULL;
    GtkWidget *treeviewLocal, *treeviewServer;
    GtkListStore *store;
    GtkTreeSelection *selection;

    //Chemins
    char *dir_nameLocal;
    char *dir_nameServer;

    //Images
    const char *pathFolder = "./../asset/directory.png";
    const char *pathFile = "./../asset/file.png";


//Ajoute le texte dans la console
void insertConsole(char *newText){
    gtk_text_buffer_get_end_iter(textBufferConsole,&iter);
//    snprintf(newText, sizeof newText,"%s", "\n");
    gtk_text_buffer_insert(textBufferConsole,&iter,"\n",-1);
    gtk_text_buffer_get_end_iter(textBufferConsole,&iter);
    gtk_text_buffer_insert(textBufferConsole,&iter,newText,-1);
}

//Clique bouton Connexion
static void connexion (GtkWidget *wid, GtkWidget *win){
	//TODO: utiliser ce qui se trouvent dans les champs editAddress et editSocket
	init_client ("localhost","8080");
	//insertConsole(g_object_get (G_OBJECT (editAddress), "editAddress", &editAddress, NULL));

/*
  GtkWidget *dialog = NULL;

  dialog = gtk_message_dialog_new (GTK_WINDOW (win), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Connexion");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  */
}

//Liste colonne
enum GColumns{
    COL_TYPE = 0,
    COL_NAME,
    COL_SIZE,
    COL_LAST_UPDATE,
    NUM_COLS
};

//Affiche la liste des fichiers dans le dossier courant
void dir_list (void){
    GDir *dir = NULL;

    //Type d'un fichier
    typedef enum {
      G_FILE_TEST_IS_REGULAR    = 1 << 0, /* TRUE si le fichier est un fichier standard (ni un lien symbolique ni un dossier) */
      G_FILE_TEST_IS_SYMLINK    = 1 << 1, /* TRUE si le fichier est un lien symbolique */
      G_FILE_TEST_IS_DIR        = 1 << 2, /* TRUE si le fichier est un dossier */
      G_FILE_TEST_IS_EXECUTABLE = 1 << 3, /* TRUE si le fichier est un executable */
      G_FILE_TEST_EXISTS        = 1 << 4  /* TRUE si le fichier existe */
    } GFileTest;

    dir = g_dir_open (dir_nameLocal, 0, NULL);
    if (dir) {
        const gchar *read_name = NULL;

        GtkTreeIter iter;
        GdkPixbuf *file_image = NULL;

        gtk_list_store_clear (store);
        file_image = gdk_pixbuf_new_from_file (pathFolder, NULL);
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, file_image, 1, "..", 2, NULL, 3, NULL, -1);

        while ((read_name = g_dir_read_name (dir))) {
            GFile *file=NULL;
            GFileInfo *info =NULL;
            gchar *file_name = NULL;
            GdkPixbuf *file_image = NULL;
//            gchar *file_size = NULL;
            GTimeVal file_lastUpdate;

            //Nom du fichier
            file_name = g_build_path (G_DIR_SEPARATOR_S, dir_nameLocal, read_name, NULL);

            //Info du fichier
            file = g_file_new_for_path(file_name);
            info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_SIZE","G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);

            //Taille du fichier
            int size = g_file_info_get_size(info);

            //Dernière modification
            g_file_info_get_modification_time(info, &file_lastUpdate);
//            g_memdup(&file_lastUpdate, sizeof(file_lastUpdate));

            //Icone du fichier
            if (g_file_test (file_name, G_FILE_TEST_IS_DIR)){       //Si dossier
                file_image = gdk_pixbuf_new_from_file (pathFolder, NULL);
                size = 0;
            } else {                                                //Sinon fichier
                file_image = gdk_pixbuf_new_from_file (pathFile, NULL);
//                sprintf(file_size, "%d octets", size);
//                itoa(size, file_size, 10);
            }

            /* Ajout d'une nouvelle entree au magasin */
            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter, 0, file_image, 1, read_name, 2, size, 3, NULL, -1);
        }

        g_dir_close (dir), dir = NULL;
    }
}


//Insère les données
//static GtkTreeModel *create_and_fill_model (void){
//    GtkTreeIter    iter;
//    GdkPixbuf *iconFolder = NULL, *iconFile = NULL;
//    GError *error = NULL;

//    iconFolder = gdk_pixbuf_new_from_file (pathFolder, &error);
//    iconFile = gdk_pixbuf_new_from_file (pathFile, &error);
//    if (error){
//        g_warning ("L\'icone ne peut être chargé : %s\n", error->message);
//        g_error_free (error);
//        error = NULL;
//    }

//    store = gtk_list_store_new (NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);
//
//    gtk_list_store_clear(store);
//
//    //Affiche liste dossier
//    dir_list();

    /* Exemple d'insertion de données dans le TreeView */
//    gtk_list_store_append (store, &iter);
//    gtk_list_store_set (store, &iter, COL_TYPE, iconFolder, COL_NAME, "Dossier 1", COL_SIZE, 0, COL_LAST_UPDATE, "31/01/2013 23:09:00", -1);
//    gtk_list_store_append (store, &iter);
//    gtk_list_store_set (store, &iter, COL_TYPE, iconFile, COL_NAME, "Fichier 1", COL_SIZE, 560, COL_LAST_UPDATE, "31/01/2013 23:59:00", -1);

//    return GTK_TREE_MODEL (store);
//}

//Créé TreeView
static GtkWidget *create_view_and_model (void){
    GtkCellRenderer     *renderer;
    GtkTreeModel        *model;
    GtkWidget           *treeview;

    treeview = gtk_tree_view_new ();

    //Colonnes
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Type", renderer, "pixbuf", COL_TYPE, NULL);
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Nom", renderer, "text", COL_NAME, NULL);
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Taille", renderer, "text", COL_SIZE, NULL);
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview), -1, "Dernière modif", renderer, "text", COL_LAST_UPDATE, NULL);

//    model = create_and_fill_model ();

    //Type de données
    store = gtk_list_store_new (NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);

    //Données
    dir_list();

    model = GTK_TREE_MODEL (store);

    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
    g_object_unref (model);

    return treeview;
}

//Remplace le texte dans la barre de status
void setStatusBar(const char *newText){
    gtk_statusbar_push(GTK_STATUSBAR(statusbar),
            gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), newText),
            newText);
}

//Sélection d'un élément d'un treeview
void  on_changed(GtkWidget *widget, gpointer statusbar) {
    GtkTreeIter iter;
    GtkTreeModel *model;
    char *value;

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
        gtk_tree_model_get(model, &iter, COL_NAME, &value,  -1);
        setStatusBar(value);
        g_free(value);
    }
}

/** MAIN **/
int main (int argc, char *argv[]) {

/** GTK+ **/
    /* Initialise GTK+ */
    g_log_set_handler ("Gtk", G_LOG_LEVEL_WARNING, (GLogFunc) gtk_false, NULL);
    gtk_init (&argc, &argv);
    g_log_set_handler ("Gtk", G_LOG_LEVEL_WARNING, g_log_default_handler, NULL);

/** WINDOW **/
    /* Création de la fenêtre principale */
    win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width (GTK_CONTAINER (win), 8);
    gtk_window_set_title (GTK_WINDOW (win), "Projet FTP");
    gtk_window_set_default_size (GTK_WINDOW (win), 650, 400);
    gtk_window_set_position (GTK_WINDOW (win), GTK_WIN_POS_CENTER);
    gtk_widget_realize (win);
    g_signal_connect (win, "destroy", gtk_main_quit, NULL);

/** STATUSBAR **/
    statusbar = gtk_statusbar_new();
    gtk_statusbar_push(GTK_STATUSBAR(statusbar),
            gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "Bienvenue"),
            "Bienvenue sur notre FTP !");


/** CONNECT **/
    /* Boite Connexion */
    boxConnect = gtk_box_new (FALSE, 6);
//    gtk_box_set_homogeneous(GTK_BOX(boxConnect),TRUE);

    /* Champ Adresse */
    editAddress=gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(editAddress),"Adresse");
    gtk_box_pack_start(GTK_BOX(boxConnect),editAddress,TRUE,TRUE,0);
    gtk_widget_grab_focus(editAddress);

    /* Champ Port */
    editSocket=gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(editSocket),"Port");
    gtk_box_pack_start(GTK_BOX(boxConnect),editSocket,TRUE,TRUE,0);

    /* Bouton Connexion */
    buttonConnect = gtk_button_new_from_stock (GTK_STOCK_CONNECT);
    g_signal_connect (G_OBJECT (buttonConnect), "clicked", G_CALLBACK (connexion), (gpointer) win);
    gtk_box_pack_start (GTK_BOX (boxConnect), buttonConnect, FALSE, FALSE, 0);


/** CONSOLE **/
    /* Affichage de la Console */
    textViewConsole = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textViewConsole),FALSE);
    textBufferConsole=gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewConsole));
    gtk_text_buffer_set_text(textBufferConsole,"Console :",-1);
//    gtk_widget_set_size_request(textViewConsole,150,-1);

    /* Scroll local */
    scrolledWindowConsole = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request(scrolledWindowConsole,150,-1);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindowConsole), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER(scrolledWindowConsole), textViewConsole);


/** TREEVIEW LOCAL **/
    dir_nameLocal = g_strdup(g_get_home_dir());

    /* TreeView local */
    treeviewLocal = create_view_and_model ();
//    gtk_widget_set_size_request(treeviewLocal,320,200);

    /* Scroll local */
    scrolledWindowLocal = gtk_scrolled_window_new (NULL, NULL);       /*init de la scrollbar*/
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindowLocal), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER(scrolledWindowLocal), treeviewLocal); /* attache le treeview a la scrollbar*/

    /* Label local */
    labelLocal = gtk_label_new("Site local :");

    /* boite local */
    boxLocal = gtk_box_new (TRUE, 6);
    gtk_box_pack_start (GTK_BOX (boxLocal), labelLocal, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (boxLocal), scrolledWindowLocal, TRUE, TRUE, 0);

    /* sélectionne une ligne */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviewLocal));
    g_signal_connect(selection, "changed", G_CALLBACK(on_changed), statusbar);



/** TREEVIEW SERVEUR **/
//    dir_nameServer = g_strdup(g_get_home_dir());

    /* TreeView serveur */
    treeviewServer = create_view_and_model ();
//    gtk_widget_set_size_request(treeviewServer,0,200);

    /* Scroll serveur */
    scrolledWindowServer = gtk_scrolled_window_new (NULL, NULL);       /*init de la scrollbar*/
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindowServer), GTK_POLICY_ALWAYS, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER(scrolledWindowServer), treeviewServer); /* attache le treeview a la scrollbar*/

    /* Label serveur */
    labelServer = gtk_label_new("Site distant :");

    /* boite serveur */
    boxServer = gtk_box_new (TRUE, 6);
    gtk_box_pack_start (GTK_BOX (boxServer), labelServer, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (boxServer), scrolledWindowServer, TRUE, TRUE, 0);

    /* sélectionne une ligne */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeviewServer));
    g_signal_connect(selection, "changed", G_CALLBACK(on_changed), statusbar);


/** PANED (TREEVIEW LOCAL/SERVEUR) **/
    /* Paned avec les TreeView Local et serveur */
    panedTreeView=gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
//    gtk_widget_set_size_request(panedTreeView,200,-1);
    gtk_paned_set_position(GTK_PANED(panedTreeView),315);
    gtk_paned_pack1(GTK_PANED(panedTreeView),boxLocal,TRUE,TRUE);
    gtk_paned_pack2(GTK_PANED(panedTreeView),boxServer,TRUE,TRUE);


/** PANED (CONSOLE/TREEVIEWS) **/
    /* Paned avec Console et TreeView */
    paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
//    gtk_widget_set_size_request(paned,200,-1);
    gtk_paned_set_position(GTK_PANED(paned),100);
    gtk_paned_pack1(GTK_PANED(paned),scrolledWindowConsole,TRUE,FALSE);
    gtk_paned_pack2(GTK_PANED(paned),panedTreeView,TRUE,FALSE);


/** CONTAINER **/
    /* Boite Conteneur */
    box = gtk_box_new (TRUE, 6);

    //Ajoute les éléments à la boite conteneur
    gtk_box_pack_start(GTK_BOX(box),boxConnect,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(box),paned,TRUE,TRUE,0);
    gtk_box_pack_end(GTK_BOX(box), statusbar, FALSE, TRUE, 1);

    gtk_container_add (GTK_CONTAINER (win), box);
    gtk_widget_show_all (win);
    gtk_main ();

    return 0;
}