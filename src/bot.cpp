#include "../include/bot.hpp"


bot::bot(){
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    
    for(int i=0;i<5;i++){
        try{
            bytes_recv = getaddrinfo(serv_addr,port, &hints, &peer_address);
            if(bytes_recv!=0){
                throw "Error trying to reach twitch's DNS\nTrying again in 2 seconds...";
            }
            else break;
        }
        catch(const char* error){
            std::cout << bytes_recv << std::endl;
        }
        sleep(2);
    }
    
    //INITIALIZING SOCKET
    socket_peer = socket(peer_address->ai_family,
    peer_address->ai_socktype, peer_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_peer)){
        std::cout << "socket() failed" << GETSOCKETERRNO() << std::endl;
    }

    //CONNECTION
    if(connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)){
        std::cout << stderr << "connect() failed " << GETSOCKETERRNO() << std::endl;
    }

    //SET TIMEOUT ON CONNECTION
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    //STATUP
    srand (time(NULL));
    raffleIsOn = false;
    
    //MODS
    mods.push_back("jpelizza");
    mods.push_back("uneaseplacebo");
    mods.push_back("ricardostoklosa");
    mods.push_back("tteknahlowg");
    mods.push_back("manakithegreat");
    mods.push_back("alucard27xxx");
    mods.push_back("utechhh");
    
    env = new Env();
    login();
}
void bot::login(){
    char pass[4096]  = "PASS ";
    char nick[4096] = "NICK ";
    char join[4096] = "JOIN ";

    strcat(pass,(env->getValue("OAUTH")).c_str());
    strcat(pass,"\n");
    strcat(nick,(env->getValue("NICK")).c_str());
    strcat(nick,"\n");
    strcat(join,(env->getValue("CHANNEL")).c_str());
    
    channel = env->getValue("CHANNEL");
    privmsg = privmsg + std::string(channel) + std::string(" :");
    
    strcat(join,"\n");

    send(socket_peer,pass,strlen(pass),0);
    send(socket_peer,nick,strlen(nick),0);
    send(socket_peer,join,strlen(join),0);
    recv(socket_peer, NULL, 4096, 0);
}
void bot::pong(){
    char data[4096]  = "PONG :tmi.twitch.tv\n";
    int bytes_sent = send(socket_peer,data,strlen(data),0);
    if(bytes_sent > 1 && devMode) std::cout << "PONG sent\n";
}
//NO CURRENT IMPLEMENTATION
void bot::ping(){
    char data[4096]  = "PING :tmi.twitch.tv\n";
    int bytes_sent = send(socket_peer,data,strlen(data),0);
    if(bytes_sent > 1 && devMode) std::cout << "PING sent\n";
}
void bot::loop(){
    while(true){
        //SETUP READS WITH SOCKET
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
        //WINDOWS
        #if !defined(_WIN32)
            FD_SET(0,&reads);
        #endif
        //CHECK IF SELECT WAS OK
        if(select(socket_peer+1, &reads, 0, 0, &timeout) < 0){
            std::cout << stderr << "select() failed\n" << GETSOCKETERRNO();
        }
        //CHECK PEER SOCKET TO SEE IF ANYTHING WAS RECEIVED
        if(FD_ISSET(socket_peer, &reads)){
            char read[4096];
            bytes_recv = recv(socket_peer, read, 4096, 0);
            //CHECK IF CONNECTION CLOSED
            if(bytes_recv < 1){
                std::cout << "Connection closed by peer\n";
                break;
            }
            //TAKES CARE OF MESSAGE
            numOnBuffer=0;
            readString = read;
            for(int i=0;read[i]!='\0';i++){
                if(read[i]=='\n') numOnBuffer++;
            }
            for(int i = 0;i<numOnBuffer;i++){
                buffer.push_back(readString.substr(0,readString.find_first_of("\n")));
                readString = readString.substr(readString.find_first_of("\n")+1);
            }
            for(auto it = buffer.begin();it!=buffer.end();it++){
                if(devMode)std::cout << "*it = " << *it << std::endl;
                if (!(*it).substr(0,4).compare("PING")) pong();
                else if((*it).find(channel) != std::string::npos){
                    msgCheck(*it);
                }
            }
            buffer.clear();
            //PRINT
            if(devMode){
                std::cout << "Received " << bytes_recv << " bytes, message is: " << std::endl << read << std::endl;
            }
            //CLEAN CHAR ARRAY
            memset(&read, 0, sizeof(read));
        }

        //CHECK PEER_SOCKET TO SEE IF IT HAS ANYTHING TO SEND
        #if defined(_WIN32)
        if(_kbhit()){
        #else
        if(FD_ISSET(0, &reads)){
        #endif
            char read[4096];
            if(!fgets(read, 4096, stdin));
            if(read[0]=='+'){
                hostCommandChecker(std::string(read));
            }
            else{
                int bytes_sent = send(socket_peer, read, strlen(read), 0);
                if(devMode){
                    std::cout << "Sent " << bytes_sent << " bytes\n";
                }
            }
        }
        checkers();
    }
}

/*
    checks for local host commands
*/
void bot::hostCommandChecker(std::string hostCommand){
    hostCommand.pop_back();
    if(!hostCommand.compare("+playlist")){
        HCplaylist();
    }
    else if(!hostCommand.compare("+skip")){
        HCskip();
    }
    else if(!hostCommand.compare("+stop")){
        HCstop();
    }
    else if(!hostCommand.compare("+play")){
        HCplay();
    }
    else if(!hostCommand.substr(0,4).compare("+vol")){
        HCvolume(hostCommand.substr(5));
    }
    else if(!hostCommand.substr(0,7).compare("+remove")){
        HCremoveFromPlaylist(atoi(hostCommand.substr(7).c_str()));
    }
    else if(!hostCommand.substr(0,12).compare("+changeOrder")){
        HCchangeOrder(hostCommand.substr(13));
    }
    return;
}

void bot::HCchangeOrder(std::string hostCommand){
    auxInt = 0;
    size_t separator = hostCommand.find(' ');
    int toChange = atoi(hostCommand.substr(0,separator).c_str());
    int changeTo = atoi(hostCommand.substr(separator).c_str());
    if(toChange==changeTo) return;
    std::cout << toChange << " " << changeTo << std::endl;
    std::tuple<std::string,std::string> auxTuple;


    for(auto it = player.requestList.begin(); it!=player.requestList.end();auxInt++,it++){
        if(auxInt==toChange){
            auxTuple = *(it);
            auxInt=0;
            for(auto jt = player.requestList.begin(); jt!=player.requestList.end();auxInt++,jt++)
            if(auxInt==changeTo){
                player.requestList.erase(it++);
                player.requestList.insert(jt++,auxTuple);
            }
        }
    }
    
}
void bot::HCplaylist(){
    auxInt = 0;
    for(auto it=player.requestList.begin();it!=player.requestList.end();it++,auxInt++){
        std::cout << auxInt << ":" << std::get<1>(*it) << std::endl;
    }
    return;
}

void bot::HCremoveFromPlaylist(int whichToRemove){
    auxInt=0;
    for(auto it=player.requestList.begin();it!=player.requestList.end();it++,auxInt++){
        if(whichToRemove == auxInt){
            player.requestList.erase(it);
            return;
        }
    }
}

void bot::HCskip(){
    player.vlcSkip();
}
void bot::HCvolume(std::string volume){
    player.vlcChangeVolume(atoi(volume.c_str()));
}
void bot::HCplay(){
    player.ableToPlay = true;
}
void bot::HCstop(){
    player.ableToPlay = false;
}

/*
A few check fucntions to keep track on raffle and vlc player
*/
void bot::checkers(){
    title = player.checkOnPlayer();
    if(title!=""){
        sendprivmsg(std::string("Tocando agora: " + title));
    }
    checkOnRaffle();
}

/*
msgCheck(std::string @_msg)
@_msg recv bytes to be parsed and processed
*/
void bot::msgCheck(std::string msgRecv){
    latestMsg = msgManager(msgRecv);
    
        if(latestMsg.text[0] == '!'){
        if(!latestMsg.text.compare("!dice")){
            Cdice(latestMsg);
        }
        else if(!latestMsg.text.compare("!pau")){
            Cdick(latestMsg);
        }
        else if(!latestMsg.text.compare("!playlist")){
            Cplaylist(latestMsg);
        }
        else if(!latestMsg.text.substr(0,4).compare("!add")){
            Crequest(latestMsg);
        }
        else if(!latestMsg.text.substr(0,4).compare("!raf")){
            Craffle(latestMsg);
        }
    }
    /*
        Fazer classe Comando com função abstrata @override executar
        cada classe tera um string pra comparação
    */    
    return;
}

/*
msgManaget(std::string @_s)
@_s unfiltered byte stream
returns struct msg with user and text filtered
*/
struct msg bot::msgManager(std::string msgRecv){
    latestMsg.user = msgRecv.substr(1,std::string(msgRecv).find('!')-1);
    latestMsg.text = msgRecv.substr(std::string(msgRecv).find(" :")+2);
    if(chatMode) std::cout << latestMsg.user <<" : " << latestMsg.text << std::endl;
    //string fix
    latestMsg.text.pop_back();
    return latestMsg;

}
/*
isAdm(std::string @_s)
@_s is user to be checked
returns true if on mods list
else returns false
*/
bool bot::isAdm(std::string user){
    for(auto i = mods.begin();i!=mods.end();i++){
        if(!(i->compare(user))){
            return true;
        }
    }
    return false;
}


void bot::Cdice(struct msg latestMsg){
    msg ="@" + latestMsg.user + " rolled " +
                        std::to_string((rand()%20)+1);
    sendprivmsg(msg);
}
void bot::Cdick(struct msg latestMsg){
    msg ="@" + latestMsg.user + " tem " +
                            std::to_string((rand()%20)+1) + "cm de pinto";
    sendprivmsg(msg);
}
void bot::Craffle(struct msg latestMsg){
    if(isAdm(latestMsg.user)){
        if(!raffleIsOn){
            time(&raffleTimer);
            raffleIsOn = true;
            raffleSeconds = atoi(latestMsg.text.substr(4).c_str());
            if(raffleSeconds>120){
                raffleSeconds=120;
            }
        }
        else{
            raffleList.push_back(latestMsg.user);
        }
    }
}
void bot::Crequest(struct msg latestMsg){
    int requestResponse = player.addToRequestList(latestMsg.text.substr(4));
    switch (requestResponse){
    case 1:
        msg = "@" + latestMsg.user + " Seu link foi negado pelo bot, por favor não usar link de radio, obrigado!";
        break;
    case 2:
        msg = "@" + latestMsg.user + " Esta música já foi adicionada na playlist, fica esperto que daqui a pouco toca jpelizMK";
        break;
    default:
        msg = "@" + latestMsg.user + " Música adicionada a playlist sucesso!!";
        break;
    }
    sendprivmsg(msg);
    return;
}
void bot::Cplaylist(struct msg latestMsg){
    auxInt=0;
    if(player.requestList.size()>0){
        msg = "Up next: ";
        for(auto it=player.requestList.begin();it!=player.requestList.end() && auxInt<3;it++,auxInt++){
            msg += std::get<1>(*it) + " ; ";
        }
        if((int(player.requestList.size()) - 3) > 0){
            msg += " e mais "+ std::to_string(player.requestList.size()-std::size_t(3)) + " outras!";
        }
    }
    else{
        msg = "A playlist esta vazia, adiciona uma música com: !add (<yt-link>|nome da música)";
    }
    sendprivmsg(msg);
    return;
}

/*
checkOnRaffle()
If raffle is running, keeps track of time and users participating
*/
void bot::checkOnRaffle(){
    if(raffleIsOn==false){
        return;
    }
    else if(difftime(time(NULL),raffleTimer) > raffleSeconds){
        std::cout << raffleList.size() << std::endl;
        raffleIsOn = false;

        if(raffleList.size() < 1) return;

        msg = "@" + raffleList.at(rand()%raffleList.size()) + " won the raffle! CONGRATULATIONS!";

        sendprivmsg(msg);

        while(raffleList.size() > 0){
            raffleList.erase(raffleList.begin());
        }
    }
}
/*
sendprivmsg(std::string @_s)
sends @_s and privsmg on connected channel
*/
void bot::sendprivmsg(std::string text){
    text = privmsg + text + "\n";
    int bytes_sent = send(socket_peer,text.c_str(),strlen(text.c_str()),0);
    if(bytes_sent<1){
        std::cout << "ERROR TRYING TO SEND MESSAGE\n";
    }
    text.clear();
}