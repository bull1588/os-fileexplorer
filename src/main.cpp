#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>
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
} ;

typedef struct AppData {
    std::string current_dir;
    std::vector<std::vector<FileData>> file_entries;
    TTF_Font *font;
    int text_column_offset;
} AppData;

void initialize(SDL_Renderer *renderer, AppData *data_ptr, int list_index);
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

    // initializing SDL as Video
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    // create window and renderer
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);

    // initialize and perform rendering loop
    initialize(renderer, &dt, 0);
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
                                initialize(renderer, &dt, 0);
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
                                initialize(renderer, &dt, 0);
                            }
                        } else {
                            std::cout << "You clicked a file of type: " << file.type << '\n';
                            std::cout << "Not ready yet!\n";
                        }
                    } 
                }
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
            if(event.key.keysym.scancode == SDL_SCANCODE_UP && dt.file_entries[0].front().icon_rect.y < HEIGHT){
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

void initialize(SDL_Renderer *renderer, AppData *data_ptr, int list_index)
{
    data_ptr->font = TTF_OpenFont("resrc/OpenSans-Regular.ttf", 20);
    SDL_Color color = { 0, 0, 0 };
    for(int i = 0; i < data_ptr->file_entries[list_index].size(); i++) {
        
        //name, create texture and init the x y w h
        const char* filename = data_ptr->file_entries[list_index][i].filename.c_str();
        SDL_Surface *text_surf = TTF_RenderText_Solid(data_ptr->font, filename, color);
        data_ptr->file_entries[list_index][i].text_texture = SDL_CreateTextureFromSurface(renderer, text_surf);
        SDL_FreeSurface(text_surf);
        data_ptr->file_entries[list_index][i].text_rect.x = 50;
        data_ptr->file_entries[list_index][i].text_rect.y = i * 24;
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
        data_ptr->file_entries[list_index][i].icon_rect.x = 24;
        data_ptr->file_entries[list_index][i].icon_rect.y = i * 24;
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
            
            //std::cout << sizeprint << '\n';

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

    // set color of background when erasing frame
    //SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
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

            SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].size_texture, NULL, &(data_ptr->file_entries[i][j].size_rect));
            SDL_RenderCopy(renderer, data_ptr->file_entries[i][j].permissions_texture, NULL, &(data_ptr->file_entries[i][j].permissions_rect));
            
        }
    }

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
    }
    file->perms += "]";

}

void convertToUsableType(FileData *file){
    std::string filename = file->filename;
    
    //search properties
    std::vector<std::string> image = {".jpg", ".jpeg", ".png", ".tif", ".tiff", ".gif"};
    std::vector<std::string> video = {".mp4", ".mov", ".mkv", ".avi", ".webm"};
    std::vector<std::string> code = {".h", ".c", ".cpp", ".py", ".java", ".js"};
    
    
    std::size_t found = filename.find(".exe"); //WRONG! NEED TO CHECK FOR X PERMS
    if(found != std::string::npos){
        file->type = "executable";
        return;
    }
    

    for (int i = 0; i < image.size(); i++) {
        std::size_t found = filename.find(image[i]);
        if (found != std::string::npos) {
            file->type = "image";
            return;
        }
    }

    for (int i = 0; i < video.size(); i++) {
        std::size_t found = filename.find(video[i]);
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
    file->type = "other";
}


