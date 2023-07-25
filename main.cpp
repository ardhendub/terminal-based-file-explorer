#include <stdio.h>
#include <string.h>
#include <string>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <pwd.h>
#include <grp.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <algorithm>
#include <signal.h>
#include <iostream>
#include <fcntl.h>

using namespace std;

void closing();
void init();
string LastName(string a);
string removeLast(string a);
string convertToString(char* a);
string formatSize(long long x);
char* convertToChar(string s);
char* trim(char* a);
long long dirSize(string file);
string relativeToAbsolute(string a);
string pathCorrection(string a);
vector<string> permi(string file,string name,int type);
void processDirectory(const char* dirname);
void listFiles(const char* dirname);
void do_resize(int signum);
void listFilesCommand(const char* dirname);
void makeToken(string a);
void gotoCommand(string a);
bool search(const char* dirname, string a);
void createDir2(string a,string b);
void createDir();
int createFile(const char* a);
void createMultipleFiles();
void rename();
int copyFile(const char* a,const char* b);
void copyFolderHandler(string from, string to);
void copyFiles();
void deleteFile(const char* a);
void deleteFolder(const char* a);
void deleteFolderHandler(string file);
void deleteFunction();
void command_mode();

int x=1,y=0;
int currentListSize=0;
string currentdir;
vector<vector<string>> vec(1,vector<string>(7));
vector<string> distrack(1);
vector<string> dirStack;
int stTop=0,stBottom=0,stCurr=0;
int terCols=0,terRows=0;
int ulim=0,blim=0;
int operation=0;
string buffer="";
int bufferSize=0;
vector<string> token;
string message="";
struct termios old_tio;
int mode;
int horlim=0;

void closing(){
      tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_tio);
}

void init(){
    uid_t id = geteuid();
    struct passwd *pwd = getpwuid(id);
    currentdir="/home/";
    currentdir+=pwd->pw_name;
    dirStack.push_back(currentdir);
    stTop=0,stBottom=0,stCurr=0;
    struct termios new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &=(~ICANON);
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
    atexit(closing);   
    signal(SIGWINCH, do_resize);
    mode=0;

}

string LastName(string a){
   for(int i=a.length();i>=0;i--){
      if(a[i]=='/'){
         return a.substr(i+1,a.length()-i);
      }
   }
}

string removeLast(string a){
    if(a=="/"){
        return a;
    }
    int n=a.length(),i;
    for(i=n-1;i>=0;i--){
        if(a[i]=='/'){
            break;
        }
    }
    if(i==-1){
        string r="";
        return r;
    }
    return a.substr(0,i).length()==0?"/":a.substr(0,i);
}

string convertToString(char* a)
{
    int i,counter=0;
    while(a[counter]!='\0'){
        counter++;
    }
    string s = "";
    for (i = 0; i < counter; i++) {
        s = s + a[i];
    }
    return s;
}

string formatSize(long long x){
    int counter=0;
    if(x==0){
        string res="0B";
        return res;
    }
    while(x>1024){
        counter++;
        x=(double)x/1024;
    }
    string res;
    if(counter==1){
        res=to_string(x);
        res+="KB";
    }
    else if(counter==2){
        res=to_string(x);
        res+="MB";
    }
    else if(counter==3){
        res=to_string(x);
        res+="GB";
    }
    else if(counter==4){
        res=to_string(x);
        res+="GB";
    }
    else if(counter==5){
        res=to_string(x);
        res+="PB";
    }
    else{
        res=to_string(x);
        res+="B";
    }
    return res;
}

char* convertToChar(string s){
    char res[s.length()+1];
    strcpy(res,s.c_str());
    return res;
}

char* trim(char* a){
    int len=0;
    while(a[len]!='\n'){
        len++;
    }
    a[len]='\0';
    return a;
}

long long dirSize(string file){
    if(file=="/"||file=="/home/.."){
        return 4096;
    }
    long long counter=0;
    DIR* dir = opendir(file.c_str());
    if (dir == NULL) {
        return 0;
    }
    struct dirent* entity;
    entity = readdir(dir);
    while (entity != NULL) {
        string ent=entity->d_name;
        if(ent=="."||ent==".."){
            entity = readdir(dir);
            continue;
        }
        else if(entity->d_type==4){
            string newFile=file+"/"+entity->d_name;
            counter+=dirSize(newFile);
        }
        else{
            struct stat st;
            string newFile=file+"/"+entity->d_name;
            stat(newFile.c_str(), &st);
            counter+=st.st_size;
        }
        entity = readdir(dir);
    }
    closedir(dir);
    return counter;
}

string relativeToAbsolute(string a){
    string newPath=currentdir;
    if(a=="."){
        return newPath;
    }
    if(a[0]=='.'&&a[1]=='.'){
        newPath+='/';
        newPath+=a;
    }
    else if(a[0]=='.'){
        newPath+=a.substr(1,a.length()-1);
    }
    else if(a[0]=='/'){
        newPath=a;
    }
    else if(a[0]=='~'){
        uid_t id = geteuid();
        struct passwd *pwd = getpwuid(id);
        newPath="/home/";
        newPath+=pwd->pw_name;
        newPath+=a.substr(1,a.length()-1);
    }
    else{
        newPath+='/';
        newPath+=a;
    }
    return newPath;
}

string pathCorrection(string a){
    string res="";
    int start=0;
    for(int i=0;i<a.length()-1;i++){
        if(a.substr(i,2)==".."){
            if(res[res.length()-1]=='/'){
                res+=removeLast(a.substr(start+1,i-start-2));
            }
            else{
                res+=removeLast(a.substr(start,i-start-1));
            }
            start=i+2;
        }
    }
    if(res[res.length()-1]=='/'&&start+1<a.length()){
        res+=a.substr(start+1,a.length()-start-2);
    }
    else if(start<a.length()){
        res+=a.substr(start,a.length()-start);
    }
    a=res;
    start=0;
    res="";
    for(int i=0;i<a.length();i++){
        if(a[i]=='.'&&a[i+1]!='/'){
            continue;
        }
        if(a[i]=='.'){
            res+=a.substr(start,i-start-1);
            start=i+1;
        }
    }
    res+=a.substr(start,a.length()-start);
    a=res;
    start=0;
    res="";
    for(int i=1;i<a.length();i++){
        if(a[i]=='/'&&a[i-1]=='/'){
            res+=a.substr(start,i-start);
            start=i+1;
        }
    }
    res+=a.substr(start,a.length()-start);
    return res;
}

vector<string> permi(string file,string name,int type){
    struct stat st;
    vector<string> filedata(7);
    if(stat(file.c_str(), &st) == 0){
        filedata[6] = to_string(type);
        filedata[0] = name;
        mode_t perm = st.st_mode;
        filedata[5] += (perm & S_IRUSR) ? "r" : "-";
        filedata[5] += (perm & S_IWUSR) ? "w" : "-";
        filedata[5] += (perm & S_IXUSR) ? "x" : "-";
        filedata[5] += (perm & S_IRGRP) ? "r" : "-";
        filedata[5] += (perm & S_IWGRP) ? "w" : "-";
        filedata[5] += (perm & S_IXGRP) ? "x" : "-";
        filedata[5] += (perm & S_IROTH) ? "r" : "-";
        filedata[5] += (perm & S_IWOTH) ? "w" : "-";
        filedata[5] += (perm & S_IXOTH) ? "x" : "-";
        if(name=="."){
            filedata[1] = formatSize(dirSize(pathCorrection(relativeToAbsolute(file))));
        }
        else if(name==".."){
            filedata[1] = formatSize(dirSize(pathCorrection(relativeToAbsolute(file))));
        }
        else if(filedata[6]=="4"){
            filedata[1] = formatSize(dirSize(pathCorrection(relativeToAbsolute(file))));
        }
        else{
            filedata[1] = formatSize(st.st_size);
        }
        filedata[2] = trim(ctime(&st.st_mtime));
        filedata[3] = getpwuid(st.st_uid)->pw_name;
        filedata[4] = getgrgid(st.st_gid)->gr_name;
        return filedata;
    }
    else{
        // return strerror(errno);
    }   
}

void processDirectory(const char* dirname){
    DIR* dir = opendir(dirname);
    if (dir == NULL) {
        message="Error!";
        return;
    }
    vec.resize(1,vector<string>(7));
    struct dirent* entity;
    entity = readdir(dir);
    while (entity != NULL) {
        string filename=dirname;
        filename+="/";
        filename+=entity->d_name;
        vec.push_back(permi(filename,convertToString(entity->d_name),entity->d_type));
        entity = readdir(dir);
    }
    closedir(dir);
    currentListSize=vec.size()-1;
    sort(vec.begin(),vec.end());
    distrack.resize(1);
    for(int i=1;i<vec.size();i++){
        string entry="";
        if(vec[i][0].length()>28){
            entry+=vec[i][0].substr(0,25)+"...";
        }
        else{
            entry+=vec[i][0];
            for(int j=0;j<28-vec[i][0].length();j++){
                entry+=" ";
            }
        }
        entry+="  ";
        entry+=vec[i][1];
        for(int j=0;j<6-vec[i][1].length();j++){
            entry+=" ";
        }
        entry+="  ";
        if(vec[i][3].length()>8){
            entry+=vec[i][3].substr(0,6)+"..";
        }
        else{
            entry+=vec[i][3];
            for(int j=0;j<8-vec[i][3].length();j++){
                entry+=" ";
            }
        }
        entry+="  ";
        if(vec[i][4].length()>8){
            entry+=vec[i][4].substr(0,6)+"..";
        }
        else{
            entry+=vec[i][4];
            for(int j=0;j<8-vec[i][4].length();j++){
                entry+=" ";
            }
        }
        entry+="  ";
        entry+=vec[i][5]+"  "+vec[i][2]+"  ";
        if(vec[i][6]=="4"){
            entry+="folder";
        }
        else{
            entry+="file  ";
        }
        distrack.push_back(entry);
    }
}

void listFiles(const char* dirname){
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    terCols=w.ws_col;
    terRows=w.ws_row;
    // printf("\x1B[1;1H\x1B[2J");
    printf("\033[H\033[2J\033[3J");
    if(currentListSize<=terRows-3&&ulim==0&&blim==0){
        ulim=1;
        blim=currentListSize;
    }
    else if(ulim==0&&blim==0){
        ulim=1;
        blim=terRows-3;
    }
    int d;
    if(x==terRows-3){
        d=ulim+terRows-4;
    }
    else{
        d=ulim+x-1;
    }
    if(horlim==0){
        horlim=terCols-2;
    }
    for(int i=ulim;i<=blim;i++){
        if(d==i){
            cout<<"\033[1;7m";
        }
        cout<<distrack[i].substr(0,horlim)<<"\n";
        cout<<"\033[0m";
    }
    printf("%c[%d;%df",0x1B,w.ws_row-1,0);
    printf(">>>>>>Normal Mode: %s",message.c_str());
    printf("%c[%d;%df",0x1B,w.ws_row,0);
    printf("%s",currentdir.c_str());
    printf("%c[%d;%df",0x1B,x,y);
    cout<<flush;
    return;
}

void do_resize(int signum){
    ulim=0;
    blim=0;
    x=1;
    horlim=0;
    if(mode==0){
        listFiles(currentdir.c_str());
    }
    else{
        listFilesCommand(currentdir.c_str());
    }
}

void listFilesCommand(const char* dirname){
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    terCols=w.ws_col;
    terRows=w.ws_row;
    // printf("\x1B[1;1H\x1B[2J");
    printf("\033[H\033[2J\033[3J");
    if(currentListSize<=terRows-3&&ulim==0&&blim==0){
        ulim=1;
        blim=currentListSize;
    }
    else if(ulim==0&&blim==0){
        ulim=1;
        blim=terRows-3;
    }
    if(horlim==0){
        horlim=terCols-2;
    }
    for(int i=ulim;i<=blim;i++){
        cout<<distrack[i].substr(0,horlim)<<"\n";
        cout<<"\033[0m";
    }
    printf("%c[%d;%df",0x1B,w.ws_row-1,0);
    printf(">>>>>>Command Mode: %s",message.c_str());
    printf("%c[%d;%df",0x1B,w.ws_row,0);
    printf("%s$ %s",currentdir.c_str(),buffer.substr(0,bufferSize).c_str());
    cout<<flush;
    return;
}

void makeToken(string a){
    token.clear();
    int start=0,end=0;
    for(int i=0;i<a.length();i++){
        if(a[i]==' '){
            end=i;
            if(a.substr(start,end-start)!=""){
                token.push_back(a.substr(start,end-start));
            }
            start=i+1;
        }
        else{
            continue;
        }
    }
    if(a.substr(start,a.length()-start)!=""){
        token.push_back(a.substr(start,a.length()-start));
    }
    return;
}

void gotoCommand(string a){
    currentdir=a;
    x=1;
}

bool search(const char* dirname, string a){
    DIR* dir = opendir(dirname);
    if (dir == NULL) {
        return false;
    }
    struct dirent* entity;
    entity = readdir(dir);
    while (entity != NULL) {
        if((entity->d_name)[0]=='.'){
            entity = readdir(dir);
            continue;
        }
        else if(entity->d_name=="."){
            entity = readdir(dir);
            continue;
        }
        else if(entity->d_name==".."){
            entity = readdir(dir);
            continue;
        }
        if(entity->d_name==a){
            message="True";
            return true;
        }
        string filename=dirname;
        if(entity->d_type==4){
            filename+="/";
            filename+=entity->d_name;
            bool b=search(filename.c_str(),a);
            if(b==true){
                return true;
            }
        }
        entity = readdir(dir);
    }
    closedir(dir);
    return false;
}

void createDir2(string a,string b){
    b+="/";
    b+=LastName(a);
    mkdir(b.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    struct stat st;
    stat(a.c_str(), &st);
    chmod(b.c_str(),st.st_mode);
}

void createDir(){
    if(token.size()<=2){
        message="Invalid number of Input!";
        return;
    }
    else{
        string folder=pathCorrection(relativeToAbsolute(token[token.size()-1]));
        for(int i=1;i<token.size()-1;i++){
            string newfolder=folder;
            newfolder+="/";
            newfolder+=token[i];
            int check=mkdir(newfolder.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
            if(check==-1){
                message="Error!";
                return;
            }
            else{
                message="Success!";
            }
        }
    }
}

int createFile(const char* a){
    int check=open(a,O_RDONLY | O_CREAT,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
    if(check==-1){
        // message="Error!";
        return 0;
    }
    else{
        // message="Success!";
        return 1;
    }
}

void createMultipleFiles(){
    for(int i=1;i<token.size();i++){
        int check=createFile(pathCorrection(relativeToAbsolute(token[i])).c_str());
        if(check==0){
            message="Error!";
            return;
        }
        else{
            message="Success!";
        }
    }
}

void rename(){
    int check=rename(pathCorrection(relativeToAbsolute(token[1])).c_str(),pathCorrection(relativeToAbsolute(token[2])).c_str());
    if(check==-1){
        message="Error!";
    }
    else{
        message="Success!!";
    }
}

int copyFile(const char* a,const char* b){
    char c[1024];
    int in, out;
    in = open(a, O_RDONLY);
    if(in==-1){
        message="Error!";
        return -1;
    }
    out = open(b, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
    if(in==-1||out==-1){
        message="Error!";
        return -1;
    }
    while(read(in,c,1024)>0){
        write(out,c,1024);
    }
    struct stat st;
    stat(a, &st);
    chmod(b,st.st_mode);
    return 0;
}

void copyFolderHandler(string from, string to){
    DIR* dir = opendir(from.c_str());
    if (dir == NULL) {
        return;
    }
    struct dirent* entity;
    entity = readdir(dir);
    createDir2(from,to);
    while (entity != NULL) {
        if(message=="Error!"){
            return;
        }
        string ent=entity->d_name;
        if(ent=="."){
            entity = readdir(dir);
            continue;
        }
        else if((entity->d_name)[0]=='.'&&(entity->d_name)[1]=='.'){
            entity = readdir(dir);
            continue;
        }
        else if(entity->d_type==4){
            string newFrom=from+"/";
            newFrom+=entity->d_name;
            string newTo=to+"/"+LastName(from);
            copyFolderHandler(newFrom,newTo);
        }
        else{
            string newFrom=from+"/";
            newFrom+=entity->d_name;
            string newTo=to+"/"+LastName(from)+"/";
            newTo+=entity->d_name;
            copyFile(newFrom.c_str(),newTo.c_str());
        }
        entity = readdir(dir);
    }
    closedir(dir);
}

void copyFiles(){
    
    string to,from;
    for(int i=1;i<token.size()-1;i++){
        to=pathCorrection(relativeToAbsolute(token[token.size()-1]));
        from=pathCorrection(relativeToAbsolute(token[i]));
        struct stat st;
        stat(from.c_str(),&st);
        if(st.st_size==4096){
            copyFolderHandler(from,to);
            if(message=="Error!"){
                return;
            }
        }
        else{
            string newTo=to+"/"+LastName(from);
            int check=copyFile(from.c_str(),newTo.c_str());
            if(check==-1){
                message="Error!";
                return;
            }
            else{
                message="Success!";
            }
        }
    }
    message="Success!";
}

void deleteFile(const char* a){
    int check=remove(a);
    if(check==-1){
        message="Error!";
    }
}

void deleteFolder(const char* a){
    int check=rmdir(a);
    if(check==-1){
        message="Error!";
    }
}

void deleteFolderHandler(string file){
    DIR* dir = opendir(file.c_str());
    if (dir == NULL) {
        return;
    }
    struct dirent* entity;
    entity = readdir(dir);
    while(entity!=NULL){
        if(message=="Error!"){
            return;
        }
        string ent=entity->d_name;
        if(ent=="."||ent==".."){
            entity = readdir(dir);
            continue;
        }
        else if(entity->d_type==4){
            string newFile=file+"/"+entity->d_name;
            deleteFolderHandler(newFile);
        }
        else{
            string newFile=file+"/"+entity->d_name;
            deleteFile(newFile.c_str());
        }
        entity = readdir(dir);
    }
    deleteFolder(file.c_str());
}

void deleteFunction(){
    string file;
    for(int i=1;i<token.size();i++){
        file=pathCorrection(relativeToAbsolute(token[i]));
        struct stat st;
        stat(file.c_str(),&st);
        if(st.st_size==4096){
            deleteFolderHandler(file);
            if(message=="Error!"){
                return;
            }
        }
        else{
            deleteFile(file.c_str());
            if(message=="Error!"){
                return;
            }
        }
    }
    message="Success!";
}

void moveFile(string from, string to){
    string newTo=to+"/"+LastName(from);
    copyFile(from.c_str(),newTo.c_str());
    if(message=="Error!"){
        return;
    }
    if(newTo==from){
        message="Success!";
        return;
    }
    deleteFile(from.c_str());
}

void moveFolder(string from,string to){
    copyFolderHandler(from,to);
    if(message=="Error!"){
        return;
    }
    deleteFolderHandler(from);
}

void moveFunction(){
    string to,from;
    for(int i=1;i<token.size()-1;i++){
        to=pathCorrection(relativeToAbsolute(token[token.size()-1]+"/"+LastName(pathCorrection(relativeToAbsolute(token[i])))));
        from=pathCorrection(relativeToAbsolute(token[i]));
        if(to==from){
            message="Folder already present!";
            return;
        }
        to=pathCorrection(relativeToAbsolute(token[token.size()-1]));
        from=pathCorrection(relativeToAbsolute(token[i]));
        struct stat st;
        stat(from.c_str(),&st);
        if(st.st_size==4096){
            moveFolder(from,to);
            if(message=="Error!"){
                return;
            }
        }
        else{
            moveFile(from,to);
            if(message=="Error!"){
                return;
            }
        }
    }
    message="Success!";
}

void command_mode(){
    char ch='0',prev='0',prev_prev='0';
    listFilesCommand(currentdir.c_str());
    while(ch=getchar()){
        if(ch==27){
            mode=0;
            return;
        }
        else if(ch!='\n'){
            if(ch==127&&bufferSize>0){
                bufferSize--;
                listFilesCommand(currentdir.c_str());
            }
            else if(ch==127&&bufferSize==0){
                listFilesCommand(currentdir.c_str());
            }
            else{
                if(buffer.length()>bufferSize){
                    buffer[bufferSize]=ch;
                    bufferSize++;
                    listFilesCommand(currentdir.c_str());
                }
                else{
                    buffer+=ch;
                    bufferSize++;    
                    listFilesCommand(currentdir.c_str());
                }
            }
        }
        else if(ch=='\n'){
            makeToken(buffer);//check for : buffer.substr(0,buffersize)
            if(token[0]=="quit"){
                printf("\033[H\033[2J\033[3J");
                printf("%c[%d;%df",0x1B,1,1);
                exit(1);
            }
            if(token.size()<2){
                message="Invalid Input!!";
                buffer="";
                bufferSize=0;
                listFilesCommand(currentdir.c_str());
            }
            else{
                if(token[0]=="goto"){
                    message="";
                    buffer="";
                    bufferSize=0;
                    string temp=currentdir;
                    gotoCommand(pathCorrection(relativeToAbsolute(token[1])));
                    x=1;
                    ulim=0;
                    blim=0;
                    processDirectory(currentdir.c_str());
                    if(message=="Error!"){
                        currentdir=temp;    
                        processDirectory(currentdir.c_str());
                    }
                    else{
                        if(stTop==stCurr){
                            dirStack.push_back(currentdir);
                            stTop++;
                            stCurr++;
                        }
                        else{
                            dirStack[stCurr+1];
                            stCurr++;
                            stTop=stCurr;
                        }
                    }
                    listFilesCommand(currentdir.c_str());
                }
                else if(token[0]=="search"){
                    message="Searching...";
                    listFilesCommand(currentdir.c_str());
                    if(search(currentdir.c_str(),token[1])==1){
                        message="True";
                    }
                    else{
                        message="False";
                    }
                    buffer="";
                    bufferSize=0;
                    ulim=0;
                    blim=0;
                    listFilesCommand(currentdir.c_str());
                }
                else if(token[0]=="create_dir"){
                    message="";
                    createDir();
                    buffer="";
                    bufferSize=0;
                    ulim=0;
                    blim=0;
                    processDirectory(currentdir.c_str());
                    listFilesCommand(currentdir.c_str());
                }
                else if(token[0]=="create_file"){
                    message="";
                    createMultipleFiles();
                    buffer="";
                    bufferSize=0;
                    ulim=0;
                    blim=0;
                    processDirectory(currentdir.c_str());
                    listFilesCommand(currentdir.c_str());
                }
                else if(token[0]=="rename"){
                    message="";
                    if(token.size()!=3){
                        message="Invalid number of input!";
                        buffer="";
                        bufferSize=0;
                        ulim=0;
                        blim=0;
                        listFilesCommand(currentdir.c_str());
                    }
                    else{
                        rename();
                        buffer="";
                        bufferSize=0;
                        ulim=0;
                        blim=0;
                        processDirectory(currentdir.c_str());
                        listFilesCommand(currentdir.c_str());
                    }
                }
                else if(token[0]=="copy"){
                    message="";
                    if(token.size()<3){
                        message="Invalid number of input!";
                        buffer="";
                        bufferSize=0;
                        ulim=0;
                        blim=0;
                        listFilesCommand(currentdir.c_str());
                    }
                    else{
                        message="Copying...";
                        listFilesCommand(currentdir.c_str());
                        copyFiles();
                        buffer="";
                        bufferSize=0;
                        ulim=0;
                        blim=0;
                        processDirectory(currentdir.c_str());
                        listFilesCommand(currentdir.c_str());
                    }
                }
                else if(token[0]=="delete_file"||token[0]=="delete_dir"){
                    message="Deleting...";
                    listFilesCommand(currentdir.c_str());
                    deleteFunction();
                    buffer="";
                    bufferSize=0;
                    ulim=0;
                    blim=0;
                    processDirectory(currentdir.c_str());
                    listFilesCommand(currentdir.c_str());
                }
                else if(token[0]=="move"){
                    message="Moving...";
                    listFilesCommand(currentdir.c_str());
                    moveFunction();
                    buffer="";
                    bufferSize=0;
                    ulim=0;
                    blim=0;
                    processDirectory(currentdir.c_str());
                    listFilesCommand(currentdir.c_str());
                }
                else{
                    message="Invalid command!";
                    buffer="";
                    bufferSize=0;
                    ulim=0;
                    blim=0;
                    listFilesCommand(currentdir.c_str());
                }

            }
        }
        else{
            listFilesCommand(currentdir.c_str());
        }
    }
}

int main(int argc, char* argv[]) {
    init();
    processDirectory(currentdir.c_str());
    listFiles(currentdir.c_str());
    char ch='0',prev='0',prev_prev='0';
    while(ch=getchar()){
        if(ch==66&&prev==91&&prev_prev==27){ //down arrow
            if(x!=currentListSize&&x!=terRows-3){
                x++;
                listFiles(currentdir.c_str());
            }
            else{
                if(blim==currentListSize){
                    listFiles(currentdir.c_str());
                }
                else{
                    ulim++;
                    blim++;
                    listFiles(currentdir.c_str());
                }   
            }
        }
        else if(ch==65&&prev==91&&prev_prev==27){ //up arrow
            if(x!=1){
                x--;
                listFiles(currentdir.c_str());
            }
            else{
                if(ulim==1){
                    listFiles(currentdir.c_str());
                }
                else{
                    ulim--;
                    blim--;
                    listFiles(currentdir.c_str());
                }
            }
        }
        else if(ch=='\n'){  //enter key
            int d;
            if(x==terRows-3){
                d=ulim+terRows-4;
            }
            else{
                d=ulim+x-1;
            }
            if(vec[d][6]=="4"){
                string temp=currentdir;
                if(vec[d][0]=="."){
                    x=1;
                    listFiles(currentdir.c_str());
                }
                else if(vec[d][0]==".."){
                    currentdir=removeLast(currentdir);
                }
                else if(currentdir!="/"){
                    currentdir+="/";
                    currentdir+=vec[d][0];
                }
                else{
                    currentdir+=vec[d][0];
                }
                x=1;
                ulim=0;
                blim=0;
                processDirectory(currentdir.c_str());
                if(message=="Error!"){
                    currentdir=temp;
                    // processDirectory(currentdir.c_str());
                }
                else{
                    if(stTop==stCurr){
                        dirStack.push_back(currentdir);
                        stTop++;
                        stCurr++;
                    }
                    else{
                        dirStack[stCurr+1];
                        stCurr++;
                        stTop=stCurr;
                    }
                }
                listFiles(currentdir.c_str());
            }
            else if(vec[d][6]=="8"){
                pid_t subprocess=fork();
                if(subprocess==0){
                    string fileName=currentdir;
                    fileName+="/";
                    fileName+=vec[d][0];
                    int check=execl("/usr/bin/xdg-open","xdg-open",fileName.c_str(),NULL);
                    if(check==-1){
                        message="Cant Open File!";
                        listFiles(currentdir.c_str());
                    }
                }
                else if(subprocess==-1){
                    printf("Error");
                }
                else{
                    listFiles(currentdir.c_str());
                }
            }
        }
        else if(ch==127){ //backspace
            currentdir=removeLast(currentdir);
            x=1;
            if(stTop==stCurr){
                dirStack.push_back(currentdir);
                stTop++;
                stCurr++;
            }
            else{
                dirStack[stCurr+1];
                stCurr++;
                stTop=stCurr;
            }
            ulim=0;
            blim=0;
            processDirectory(currentdir.c_str());
            listFiles(currentdir.c_str());
        }
        else if(ch==68&&prev==91&&prev_prev==27){ // left arrow
            if(stCurr!=0){
                stCurr--;
                x=1;
                currentdir=dirStack[stCurr];
                ulim=0;
                blim=0;
                processDirectory(currentdir.c_str());
                listFiles(currentdir.c_str());
            }
            else{
                listFiles(currentdir.c_str());
            }
        }
        else if(ch==67&&prev==91&&prev_prev==27){ //right arrow
            if(stCurr!=stTop){
                stCurr++;
                x=1;
                currentdir=dirStack[stCurr];
                ulim=0;
                blim=0;
                processDirectory(currentdir.c_str());
                listFiles(currentdir.c_str());
            }
            else{
                listFiles(currentdir.c_str());
            }
        }
        else if(ch=='q'){ // q key: quit
            printf("\033[H\033[2J\033[3J");
            printf("%c[%d;%df",0x1B,1,1);
            exit(1);
        }
        else if(ch==':'){ // ; key command mode
            mode=1;
            buffer="";
            bufferSize=0;
            message="";
            command_mode();
            listFiles(currentdir.c_str());
        }
        else if(ch=='h'){ // home key
            uid_t id = geteuid();
            struct passwd *pwd = getpwuid(id);
            currentdir="/home/";
            currentdir+=pwd->pw_name;
            x=1;
            if(stTop==stCurr){
                dirStack.push_back(currentdir);
                stTop++;
                stCurr++;
            }
            else{
                dirStack[stCurr+1];
                stCurr++;
                stTop=stCurr;
            }
            ulim=0;
            blim=0;
            processDirectory(currentdir.c_str());
            listFiles(currentdir.c_str());
        }
        else{
            listFiles(currentdir.c_str());
        }
    prev_prev=prev;
    prev=ch;
    message="";
    }
    return 0;
}
