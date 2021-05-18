#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <sstream>

#define WIDTH 800
#define HEIGHT 600

//STEPS
//store current directory (start at HOME)
//get list of all files in directory + make sure you know absolute filepath to open (xdg-open, fork() and exec() new programs)
//get info for each file (type + icon, name, size) (array of file structs?)
//display alphabetically, include ..
//all SDL functionality (one click opens file, scroll bar or other concatenation, info columns)
//recursive viewer (tree expansion of subdirectories) with toggle

class FileData { //basic file info
    public:
        SDL_Texture *text_texture;
        SDL_Texture *icon_texture;
        SDL_Texture *size_texture;
        SDL_Texture *permissions_texture;
        SDL_Rect text_rect;
        SDL_Rect icon_rect;
        SDL_Rect size_rect;
        SDL_Rect permissions_rect;

        std::string filename;
        std::string type;
        std::string path;
        double size;
        std::string perms;
        std::string units;
        int child_index; 
} ;

typedef struct AppData {
    std::string current_dir;
    std::vector<std::vector<FileData>> file_entries;
    TTF_Font *font;
    int text_column_offset;
    SDL_Texture *help_text;
    SDL_Texture *recur_texture;
    SDL_Rect recur_rect;
    SDL_Rect help_rect;
    bool recursion_switch;
} AppData;

void initialize(SDL_Renderer *renderer, AppData *data_ptr, int list_index, int y_parent, int indentation);
void static_init(SDL_Renderer *renderer, AppData *data_ptr);
void recursiveInit(SDL_Renderer *renderer, AppData *data_ptr, int list_index, int indentation_mult, int parent_y);
void render(SDL_Renderer *renderer, AppData *dt);
void updateFileList(std::vector<FileData> *files, std::string filepath);
void fitFilesizeToUnit(FileData *file);
void setFilePermField(FileData *file, struct stat *filestats);
void convertToUsableType(FileData *file);

bool compareFunction (const FileData &a, const FileData &b) 
{
    std::string _a = a.filename;
    std::string _b = b.filename;
    transform(_a.begin(), _a.end(), _a.begin(), ::tolower);
    transform(_b.begin(), _b.end(), _b.begin(), ::tolower);

    //std::cout << _a << " " << _b << '\n';

    return _a < _b;
} 

int main(int argc, char **argv)
{
    AppData dt;
    char *home = getenv("HOME");
    dt.current_dir = home;
    printf("HOME: %s\n", home);
    int filesys_idx = 0;
    std::vector<FileData> first;
    dt.file_entries.push_back(first);
    updateFileList(&dt.file_entries[0], home);
    dt.text_column_offset = 0;
    dt.recursion_switch = false;

    // initializing SDL as Video
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    // create window and renderer
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);

    // initialize and perform rendering loop
    initialize(renderer, &dt, 0, 0, 0);
    static_init(renderer, &dt);
    render(renderer, &dt);
    SDL_Event event;
    SDL_WaitEvent(&event);

    int rendercount = 0;
    while (event.type != SDL_QUIT)
    {
        //render(renderer);
        SDL_WaitEvent(&event);
        if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
            int y_clicked = event.button.y; //keep in mind that this is 'from the top' y, so inverted
            int x_clicked = event.button.x; //width on page from left

            //std::cout << "X: " << x_clicked << ", Y: " << y_clicked << '\n';
            
            if(y_clicked < HEIGHT && x_clicked < WIDTH){
                //std::cout << "checking for hit...\n";
                for(int i = 0; i < dt.file_entries[0].size(); i++){
                    FileData file = dt.file_entries[0][i];
                    int lowx = file.icon_rect.x;
                    int lowy = file.icon_rect.y;
                    int highx = file.text_rect.x + file.text_rect.w;
                    int highy = file.text_rect.y + file.text_rect.h;
                    
                    if(y_clicked > lowy && y_clicked < highy
                        && x_clicked > lowx && x_clicked < highx) //clicked in region
                    {
                        if(file.type == "directory"){
                            if(file.filename != ".."){
                                dt.current_dir += "/" + file.filename;
                                std::cout << dt.current_dir << '\n';
                                updateFileList(&dt.file_entries[0], const_cast<char*>(dt.current_dir.c_str()));
                                initialize(renderer, &dt, 0, 0, 0);
                            } else {
                                std::cout << "Going up." << '\n';
                                size_t found = dt.current_dir.find_last_of("/");
                                if(found != std::string::npos){
                                    dt.current_dir = dt.current_dir.substr(0, found); 
                                    if(dt.current_dir.empty()){
                                        dt.current_dir += "/";
                                    }
                                    std::cout << dt.current_dir << '\n';
                                }
                                updateFileList(&dt.file_entries[0], const_cast<char*>(dt.current_dir.c_str()));
                                initialize(renderer, &dt, 0, 0, 0);
                            }
                        } else {
                            std::cout << "You clicked a file of type: " << file.type << '\n';
                            std::cout << "Not ready yet!\n";
                            int pid = fork();
                            if(pid == 0){ 
                                std::string filepath = dt.current_dir + "/" + file.filename; 
                                char *const argv_list[] = {"xdg-open", const_cast<char*>(filepath.c_str()), NULL} ;
                                execvp("xdg-open", argv_list);
                            }
                        }
                    } 
                }
            }


            //CHECK FOR RECURSION TOGGLE
            if(y_clicked >= dt.recur_rect.y && y_clicked <= dt.recur_rect.y + dt.recur_rect.h 
                && x_clicked >= dt.recur_rect.x && x_clicked <= dt.recur_rect.x + dt.recur_rect.w ){
                    if(dt.recursion_switch){
                        dt.recursion_switch = false;
                    } else {
                        dt.recursion_switch = true;
                    }
                static_init(renderer, &dt);
            }

        }

        if(event.type == SDL_KEYDOWN){ //arrow keys to scroll through entries
            if(event.key.keysym.scancode == SDL_SCANCODE_DOWN && dt.file_entries[0].back().icon_rect.y > HEIGHT){
                for(int i = 0; i < dt.file_entries[0].size(); i++){
                    dt.file_entries[0][i].icon_rect.y -= HEIGHT;
                    dt.file_entries[0][i].text_rect.y -= HEIGHT;
                    dt.file_entries[0][i].permissions_rect.y -= HEIGHT;
                    dt.file_entries[0][i].size_rect.y -= HEIGHT;
                }
            }
            if(event.key.keysym.scancode == SDL_SCANCODE_UP && dt.file_entries[0].front().icon_rect.y < 0){
                for(int i = 0; i < dt.file_entries[0].size(); i++){
                    dt.file_entries[0][i].icon_rect.y += HEIGHT;
                    dt.file_entries[0][i].text_rect.y += HEIGHT;
                    dt.file_entries[0][i].permissions_rect.y += HEIGHT;
                    dt.file_entries[0][i].size_rect.y += HEIGHT;
                }
            }
        }
        rendercount++;
        //std::cout << rendercount << '\n';
        render(renderer, &dt);
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    IMG_Quit();
    TTF_Quit();

    return 0;
}

void initialize(SDL_Renderer *renderer, AppData *data_ptr, int list_index, int y_parent, int indent)
{
    data_ptr->font = TTF_OpenFont("resrc/OpenSans-Regular.ttf", 20);
    SDL_Color color = { 0, 0, 0 };
    for(int i = 0; i < data_ptr->file_entries[list_index].size(); i++) {
        
        //name, create texture and init the x y w h
        const char* filename = data_ptr->file_entries[list_index][i].filename.c_str();
        SDL_Surface *text_surf = TTF_RenderText_Solid(data_ptr->font, filename, color);
        data_ptr->file_entries[list_index][i].text_texture = SDL_CreateTextureFromSurface(renderer, text_surf);
        SDL_FreeSurface(text_surf);
        data_ptr->file_entries[list_index][i].text_rect.x = 50 + indent;
        data_ptr->file_entries[list_index][i].text_rect.y = y_parent + (i * 24);
        SDL_Rect hold = data_ptr->file_entries[list_index][i].text_rect;
        SDL_QueryTexture(data_ptr->file_entries[list_index][i].text_texture, NULL, NULL, &(data_ptr->file_entries[list_index][i].text_rect.w), &(data_ptr->file_entries[list_index][i].text_rect.h));


        if(data_ptr->file_entries[list_index][i].text_rect.w> data_ptr->text_column_offset){
            std::cout << "UPDATE LARGEST WIDTH TO: " << data_ptr->file_entries[list_index][i].text_rect.w << '\n';
            data_ptr->text_column_offset = data_ptr->file_entries[list_index][i].text_rect.w;
        }

        //icon, create texture and init the x y w h
        SDL_Surface *icon_surf;
        if (data_ptr->file_entries[list_index][i].type == "directory") {
            icon_surf = IMG_Load("resrc/images/dir.png");
        }else if(data_ptr->file_entries[list_index][i].type == "executable") {
            icon_surf = IMG_Load("resrc/images/exe.png");
        }else if(data_ptr->file_entries[list_index][i].type == "image") {
            icon_surf = IMG_Load("resrc/images/img.png");
        }else if(data_ptr->file_entries[list_index][i].type == "video") {
            icon_surf = IMG_Load("resrc/images/vid.png");
        }else if(data_ptr->file_entries[list_index][i].type == "code") {
            icon_surf = IMG_Load("resrc/images/code.png");
        }else if(data_ptr->file_entries[list_index][i].type == "other") {
            icon_surf = IMG_Load("resrc/images/oth.png");
        }
        data_ptr->file_entries[list_index][i].icon_texture = SDL_CreateTextureFromSurface(renderer, icon_surf);
        SDL_FreeSurface(icon_surf);
        data_ptr->file_entries[list_index][i].icon_rect.x = 24 + indent;
        data_ptr->file_entries[list_index][i].icon_rect.y = y_parent + (i * 24);
        data_ptr->file_entries[list_index][i].icon_rect.w = 24;
        data_ptr->file_entries[list_index][i].icon_rect.h = 24;
        //SDL_QueryTexture(data_ptr->file_entries[list_index][i].icon_texture, NULL, NULL, &(data_ptr->file_entries[list_index][i].icon_rect.w), &(data_ptr->file_entries[list_index][i].icon_rect.h));
    }

    std::cout << data_ptr->text_column_offset << '\n';

    //handle permissions and sizes, if needed
    for(int i = 0; i < data_ptr->file_entries[list_index].size(); i++) { 
        if (!(data_ptr->file_entries[list_index][i].type == "directory")) { //if it is a file, sizes
            FileData cur = data_ptr->file_entries[list_index][i];
            std::stringstream stream;
            stream << data_ptr->file_entries[list_index][i].size;
            std::string fsize = stream.str() + " " + cur.units;
            
            std::cout << "setting up perm prints for" << cur.filename << '\n';
            std::cout << "  should read: " << fsize << "for size\n";
            std::cout << "  should read" << cur.perms << "for perms\n";
            std::cout << "OFFSET: " << data_ptr->text_column_offset << '\n';

            SDL_Surface *size_surf = TTF_RenderText_Solid(data_ptr->font, fsize.c_str(), color);
            data_ptr->file_entries[list_index][i].size_texture = SDL_CreateTextureFromSurface(renderer, size_surf);
            SDL_FreeSurface(size_surf);
            data_ptr->file_entries[list_index][i].size_rect.x = 100 + data_ptr->text_column_offset;
            data_ptr->file_entries[list_index][i].size_rect.y = i * 24;
            SDL_Rect hold = data_ptr->file_entries[list_index][i].size_rect;
            SDL_QueryTexture(data_ptr->file_entries[list_index][i].size_texture, NULL, NULL, &(data_ptr->file_entries[list_index][i].size_rect.w), &(data_ptr->file_entries[list_index][i].size_rect.h));

            SDL_Surface *perm_surf = TTF_RenderText_Solid(data_ptr->font, cur.perms.c_str(), color);
            data_ptr->file_entries[list_index][i].permissions_texture = SDL_CreateTextureFromSurface(renderer, perm_surf);
            SDL_FreeSurface(perm_surf);
            data_ptr->file_entries[list_index][i].permissions_rect.x = 250 + data_ptr->text_column_offset;
            data_ptr->file_entries[list_index][i].permissions_rect.y = i * 24;
            SDL_Rect permhold = data_ptr->file_entries[list_index][i].permissions_rect;
            SDL_QueryTexture(data_ptr->file_entries[list_index][i].permissions_texture, NULL, NULL, &(data_ptr->file_entries[list_index][i].permissions_rect.w), &(data_ptr->file_entries[list_index][i].permissions_rect.h));

        }
    }
    data_ptr->text_column_offset = 0;

    // set color of background when erasing frame
    //SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
}

void recursiveInit(SDL_Renderer *renderer, AppData *data_ptr, int list_index, int indentation_mult, int parent_y){ //loops through and initializes all folder vectors.
    //for each folder we find in our current directory, create a NEW std::vector<FileData>
    //Consider adding a field to FileData for directories called "child index";
    //list index indicates what directory we are expanding. This function runs repeatedly until list_index reaches the end of the populated dt vector.
    for(int i = 0; i < data_ptr->file_entries[list_index].size(); i++){ //ASSEMBLY LOOP
        if(data_ptr->file_entries[list_index][i].type == "directory"){
            std::vector<FileData> child;
            std::string filepath = data_ptr->current_dir + "/" + data_ptr->file_entries[0][i].filename;
            updateFileList(&child, filepath);
            data_ptr->file_entries.push_back(child);
            data_ptr->file_entries[0][i].child_index = data_ptr->file_entries.size() - 1; //for positioning on init. 
        }
    }

    for(int i = 0; i < data_ptr->file_entries[list_index].size(); i++){ //POPULATION LOOP
        if(data_ptr->file_entries[list_index][i].type == "directory"){
            int indentation  = data_ptr->file_entries[list_index][i].icon_rect.x + 30; //increment indentation by 30 off parent.
            initialize(renderer, data_ptr, data_ptr->file_entries[list_index][i].child_index, data_ptr->file_entries[list_index][i].icon_rect.y, indentation);
        } 
    }   
}

void static_init(SDL_Renderer *renderer, AppData *data_ptr){
    SDL_Surface *help_surf;
    help_surf = IMG_Load("resrc/images/tips.png");
    data_ptr->help_text = SDL_CreateTextureFromSurface(renderer, help_surf);
    SDL_FreeSurface(help_surf);
    data_ptr->help_rect.x = WIDTH - 90;
    data_ptr->help_rect.y = HEIGHT - 175;
    data_ptr->help_rect.w = 80;
    data_ptr->help_rect.h = 160;

    SDL_Surface *recur_surf;
    if(data_ptr->recursion_switch){
        recur_surf = IMG_Load("resrc/images/recur_on.png");
    } else {
        recur_surf = IMG_Load("resrc/images/recur_off.png");
    }
    data_ptr->recur_texture = SDL_CreateTextureFromSurface(renderer, recur_surf);
    SDL_FreeSurface(recur_surf);
    data_ptr->recur_rect.x = WIDTH - 100;
    data_ptr->recur_rect.y = 10;
    data_ptr->recur_rect.h = 100;
    data_ptr->recur_rect.w = 100;
    
    
}

void render(SDL_Renderer *renderer, AppData *data_ptr)
{
    // erase renderer content
    SDL_SetRenderDrawColor(renderer, 185, 185, 185, 185);
    SDL_RenderClear(renderer);
    
    // TODO: draw!
    //std::cout << "Writing " << data_ptr->file_entries.size() << " entries to screen\n";
    for (int i=0; i<data_ptr->file_entries.size(); i++) {
        for (int j=0; j<data_ptr->file_entries[i].size(); j++) {
            //std::cout << "Filename: " << data_ptr->file_entries[i][j].filename << '\n';
            //std::cout << "Type: " << data_ptr->file_entries[i][j].type << '\n';
            //std::cout << "Icon Rect Pos: " << data_ptr->file_entries[i][j].icon_rect.x << " , " << data_ptr->file_entries[i][j].icon_rect.y << '\n';
            //std::cout << "Text Rect Pos: " << data_ptr->file_entries[i][j].icon_rect.x << " , " << data_ptr->file_entries[i][j].icon_rect.y << '\n';
            SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].icon_texture, NULL, &(data_ptr->file_entries[i][j].icon_rect));

            //SDL_Rect rect = data_ptr->file_entries[i][j].text_rect;
            SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].text_texture, NULL, &(data_ptr->file_entries[i][j].text_rect));

            if(data_ptr->file_entries[i][j].type != "directory"){
                //std::cout << "Rendering size & perms for: " << data_ptr->file_entries[i][j].filename << '\n';
                SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].size_texture, NULL, &(data_ptr->file_entries[i][j].size_rect));
                SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].permissions_texture, NULL, &(data_ptr->file_entries[i][j].permissions_rect));
                //std::cout << SDL_GetError() << '\n';
                //SDL_ClearError();
            }
            
        }
    }

    SDL_RenderCopy(renderer, data_ptr->help_text, NULL, &(data_ptr->help_rect));
    SDL_RenderCopy(renderer, data_ptr->recur_texture, NULL, &(data_ptr->recur_rect));

    // show rendered frame
    SDL_RenderPresent(renderer);
}


void updateFileList(std::vector<FileData> *files, std::string filepath){ //called at start and whenever a new folder is expanded/opened
    
    DIR *dir;
    struct stat current_file_info;
    struct dirent *entry;
    
    //flush vector of prev info if it exists
    files->clear();
    
    if((dir = opendir (filepath.c_str())) != NULL){
        while((entry = readdir(dir)) != NULL){
            if(entry->d_type == DT_REG){ //regular file
                FileData file;
                file.filename = entry->d_name;
                file.type = entry->d_type;
                struct stat filestats;

                std::string fpath = (filepath + "/" + entry->d_name);
                std::cout << fpath << '\n';
                stat(fpath.c_str(), &filestats); //get more detailed information on the file.

                file.size = filestats.st_size;
                std::cout << "Size: " << filestats.st_size << '\n';
                fitFilesizeToUnit(&file);
                setFilePermField(&file, &filestats);
                convertToUsableType(&file);
                //std::cout << "Size: " << file.size << " " << file.units << '\n';
                files->push_back(file);

            } else if(entry->d_type == DT_DIR){
                FileData folder;
                folder.filename = entry->d_name;
                folder.type = "directory";
                if(folder.filename != "."){
                    files->push_back(folder); //add folder w/name & type into list
                }
            } else {
                std::cout << "dirent found unknown filetype.\n";
            }
        }
    }

    std::sort(files->begin(), files->end(), compareFunction);
}

void fitFilesizeToUnit(FileData *file){
    if (file->size >= 1024 && file->size  < 1048576) {
        file->size = file->size / 1024;
        file->units  = "KB";
    }else if (file->size >= 1048576 && file->size < 1073741824) {
        file->size  = file->size  / 1048576;
        file->units  = "MB";
    }else if (file->size  >= 1073741824) {
        file->size  = file->size  / 1073741824;
        file->units  = "GB";
    }else {
        file->units  = "B";
    }
    file->size = trunc(file->size);
    std::cout << trunc(file->size) << '\n';
}

void setFilePermField(FileData *file, struct stat *filestats){
    file->perms = "[";
    //Permissions
    if( filestats->st_mode & S_IRUSR ){
        file->perms += "r";
    }
    if( filestats->st_mode & S_IWUSR ){
        file->perms += "w";
    }
    if( filestats->st_mode & S_IXUSR ){
        file->perms += "x";
        file->type = "executable";
    }
    file->perms += "] | [";
    if( filestats->st_mode & S_IRGRP ){
        file->perms += "r";
    }
    if( filestats->st_mode & S_IWGRP ){
        file->perms += "w";
    }
    if( filestats->st_mode & S_IXGRP ){
        file->perms += "x";
        file->type = "executable";
    }
    file->perms += "] | [";
    if( filestats->st_mode & S_IROTH ){
        file->perms += "r";
    }
    if( filestats->st_mode & S_IWOTH ){
        file->perms += "w";
    }
    if( filestats->st_mode & S_IXOTH ){
        file->perms += "x";
        file->type = "executable";
    }
    file->perms += "]";

}

void convertToUsableType(FileData *file){
    std::string filename = file->filename;
    
    //search properties
    std::vector<std::string> img = {".jpg", ".jpeg", ".png", ".tif", ".tiff", ".gif"};

    std::vector<std::string> vid = {".mp4", ".mov", ".mkv", ".avi", ".webm"};

    std::vector<std::string> code = {".h", ".c", ".cpp", ".py", ".java", ".js"};

    

    for (int i = 0; i < img.size(); i++) {
        std::size_t found = filename.find(img[i]);
        if (found != std::string::npos) {
            file->type = "image";
            return;
        }
    }

    for (int i = 0; i < vid.size(); i++) {
        std::size_t found = filename.find(vid[i]);
        if (found != std::string::npos) {
            file->type = "video";
            return;
        }
    }

    for (int i = 0; i < code.size(); i++) {
        std::size_t found = filename.find(code[i]);
        if (found!=std::string::npos) {
            file->type = "code";
            return;
        }
    }

    if(file->type == "executable"){
        return;
    }

    file->type = "other";
}
