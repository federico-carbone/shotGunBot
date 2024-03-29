﻿#include "cppBot.h"

using namespace std;
using namespace TgBot;

long answeringAt;
int answeringMessageId, BOT_ID;

int main(){
	char* key = getenv("SHOTGUNBOTKEY");
	const string token(key);
	vector<myUser> users;
	loadUsersFromFile(&users);
	vector<Shotgun> shotguns;
	Bot bot(token);
	BOT_ID = bot.getApi().getMe()->id;

	bot.getEvents().onCommand("start", [&users, &bot](Message::Ptr message) {
		try{handleStartCommand(&users, &bot, message);}
		catch(exception& e){cerr << e.what() << endl;}
		});
	bot.getEvents().onCommand("feedback", [&users, &bot](Message::Ptr message) {
		try{handleFeedbackCommand(&users, &bot, message);}
		catch(exception& e){cerr << e.what() << endl;}
		});
	bot.getEvents().onCommand("cancel", [&users, &bot](Message::Ptr message) {
		try{handleCancelCommand(&users, &bot, message);}
		catch(exception& e){cerr << e.what() << endl;}
		});
	bot.getEvents().onCommand("create", [&shotguns, &users, &bot](Message::Ptr message) {
		try{handleCreateShotgunCommand(&shotguns, &users, &bot, message);}
		catch(exception& e){cerr << e.what() << endl;}
		});
	bot.getEvents().onCommand("reset", [&shotguns,&users, &bot](Message::Ptr message) {
		try{handleResetCommand(&shotguns, &users, &bot, message);}
		catch(exception& e){cerr << e.what() << endl;}
		});
	bot.getEvents().onNonCommandMessage([&users, &bot](Message::Ptr message) {
		try{handleNonCommand(&users, &bot, message);}
		catch(exception& e){cerr << e.what() << endl;}
		});
	bot.getEvents().onCallbackQuery([&shotguns, &users, &bot](CallbackQuery::Ptr callback) {
		try{handleCallbackQuery(&shotguns, &users, &bot, callback);}
		catch(exception& e){
			cerr << e.what() << endl;
			try{
				bot.getApi().answerCallbackQuery(callback->id, "Errore nell'elaborazione", true);
			} catch(exception& e){}
		}
		});
		
	try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        bot.getApi().deleteWebhook();

        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (exception& e) {
        printf("error: %s\n", e.what());
    }
	return 0;
}

void loadUsersFromFile(vector<myUser> *v){
	ifstream file;
	long _chatId;
	int _id;

	char cwd[50];
	getcwd(cwd, sizeof(char)*50);

	file.open(string(cwd) + "/cppBot/users.txt");
	if(file.fail()){
		cerr << "Errore nell'apertura del file in lettura" << endl;
		return;
	}
	while(file >> _chatId >> _id){
		v->push_back(myUser(_chatId, _id));
	}
	file.close();

	cout << "loaded " << v->size() << endl;
}

void addUserToFile(myUser* u){
	ofstream file;
	
	char cwd[50];
	getcwd(cwd, sizeof(char)*50);

	file.open(string(cwd) + "/cppBot/users.txt", ofstream::app);
	if(file.fail()){
		cerr << "Errore nell'apertura del file in scrittura" << endl;
		return;
	}
	file << u->chatId << " " << u->id << endl;
	cout << "new user saved" << endl;
	file.close();
}

int findShotgun(vector<Shotgun>* shotguns, Shotgun* _shotgun){
	for (int i = 0; i < shotguns->size(); i++)
	{
		if(shotguns->at(i).equals(_shotgun))
			return i;
	}
	return -1;
}

int getOrInsertUser(vector<myUser>* users, myUser* _user){
	int i;
	for (i = 0; i < users->size(); i++)
	{
		if(users->at(i).equals(_user))
			return i;
	}
	users->push_back(*_user);
	addUserToFile(_user);
	return i;
}

void handleStartCommand(vector<myUser>* users, Bot* bot, Message::Ptr message){
	myUser* tmp = new myUser(message->chat->id, message->from->id);
	int userIndex = getOrInsertUser(users, tmp);
	delete tmp;
	myUser* user = &(users->at(userIndex));
	bot->getApi().sendMessage(user->chatId, "Ciao, sono un bot demmerda scritto in c++, porta pazienza"
		" e segnala i bug tramite il comando /feedback\n\n"
		"Digita /create seguito da una serie di numeri delimitato da spazio"
		" che rappresentano il numero di sedili per fila.\n"
		"Ad esempio una multipla sarà\n/create 3 3");
}

void handleFeedbackCommand(vector<myUser>* users, TgBot::Bot* bot, TgBot::Message::Ptr message){
	myUser* tmp = new myUser(message->chat->id, message->from->id);
	int userIndex = getOrInsertUser(users, tmp);
	delete tmp;
	myUser* user = &(users->at(userIndex));
	if(message->text.size()>9){
		receivedFeedback(bot, user, message);
	} else {
		user->state = FEEDBACK;
		bot->getApi().sendMessage(user->chatId, "Invia il tuo feedback o digita /cancel per terminare");
	}
}

void handleCancelCommand(vector<myUser>* users, TgBot::Bot* bot, TgBot::Message::Ptr message){
	myUser* tmp = new myUser(message->chat->id, message->from->id);
	int userIndex = getOrInsertUser(users, tmp);
	delete tmp;
	myUser* user = &(users->at(userIndex));
	if(user->state != NORMAL){
		user->state = NORMAL;
		bot->getApi().sendMessage(user->chatId, "Operazione annullata");
	} else {
		bot->getApi().sendMessage(user->chatId, "Nessuna operazione da annullare");
	}
}

void handleCreateShotgunCommand(vector<Shotgun>* shotguns, vector<myUser>* users, TgBot::Bot* bot, TgBot::Message::Ptr message){
	string infoMessage = "Digita /create seguito da una serie di numeri delimitato da spazio"
		" che rappresentano il numero di sedili per fila.\n"
		"Aggiungi le *informazioni utili* con il parametro *-i [info]*\n "
		"Ad esempio una multipla sarà\n\n/create 3 3 -i 7.15 in stazione";

	myUser* tmp = new myUser(message->chat->id, message->from->id);
	int userIndex = getOrInsertUser(users, tmp);
	delete tmp;
	myUser* user = &(users->at(userIndex));

	int iParamIndex = message->text.find(" -i ");
	
	vector<string> options = StringTools::split(message->text.substr(0, iParamIndex), ' ');
	options.erase(options.begin());

	if(options.size() == 0){
		bot->getApi().sendMessage(user->chatId, infoMessage, false, 0, NULL, "Markdown");
		return;
	}

	Shotgun* s = new Shotgun(user->chatId, user->id);
	int shotgunIndex = findShotgun(shotguns, s);

	if(shotgunIndex!=-1){
		bot->getApi().sendMessage(user->chatId, "E' concesso un solo shotgun per utente, concludi quello che hai già iniziato e riprova");
		return;
	}

	shotguns->push_back(*s);
	shotgunIndex = shotguns->size()-1;
	delete s;
	Shotgun* shotgun = &(shotguns->at(shotgunIndex));

	shotgun->keyboard = InlineKeyboardMarkup::Ptr(new InlineKeyboardMarkup);
	string info = iParamIndex!=-1 ? "*Info: *" + message->text.substr(iParamIndex+4) : "";
	for(int i=0; i<options.size(); i++){
		int sedili;
		try{
			sedili = stoi(options[i]);
		} catch (exception& ex) {
			shotguns->pop_back();
			bot->getApi().sendMessage(user->chatId, infoMessage, false, 0, NULL, "Markdown");
			return;
		}

		vector<InlineKeyboardButton::Ptr> row(sedili);
		row.clear();
		for(int j=0; j<sedili; j++){
			InlineKeyboardButton::Ptr btn(new InlineKeyboardButton);
			if(i == 0 && j==0) btn->text = "@" + message->from->username;
			else btn->text = "Libero";
			btn->callbackData = "shotgun;place;" + to_string(shotgun->chatId) + ";" + to_string(shotgun->creatorId) + ";" + to_string(i) + ";" + to_string(j);
			row.push_back(btn);
		}
		shotgun->keyboard->inlineKeyboard.push_back(row);
	}

	vector<InlineKeyboardButton::Ptr> lRow;
	InlineKeyboardButton::Ptr btn(new InlineKeyboardButton);
	btn->text = "Concludi";
	btn->callbackData = "shotgun;stop;" + to_string(shotgun->chatId) + ";" + to_string(shotgun->creatorId);
	lRow.push_back(btn);
	shotgun->keyboard->inlineKeyboard.push_back(lRow);

	string mex = "*Attenzione Attenzione!*\n"
	"Pare che @" + message->from->username + " voglia offrire un passaggio\n"
	+ info + "\n"
	"Pronti... Ai vostri posti... *SHOTGUN*";

	Message::Ptr m = bot->getApi().sendMessage(user->chatId, mex, false, 0, shotgun->keyboard, "Markdown");
	shotgun->messageId = m->messageId;
	shotgun->messageText = mex;
	if(message->chat->type == Chat::Type::Group){
		try{bot->getApi().pinChatMessage(user->chatId, shotgun->messageId);}
		catch(exception& e){}
	}
}

void handleResetCommand(vector<Shotgun>* shotguns, vector<myUser>* users, TgBot::Bot* bot, TgBot::Message::Ptr message){
	myUser* tmp = new myUser(message->chat->id, message->from->id);
	int userIndex = getOrInsertUser(users, tmp);
	delete tmp;
	myUser* user = &(users->at(userIndex));
	for(int i=0; i<shotguns->size(); i++){
		if(shotguns->at(i).equals(Shotgun(user->chatId, user->id))){
			bot->getApi().deleteMessage(message->chat->id, shotguns->at(i).messageId);
			shotguns->erase(shotguns->begin()+i);
		}
	}
	bot->getApi().sendMessage(message->chat->id, "Reset avvenuto con successo, eliminate tutte le istanze shotgun aperte in questa chat");
}

void handleNonCommand(vector<myUser>* users, TgBot::Bot* bot, TgBot::Message::Ptr message){
	myUser* tmp = new myUser(message->chat->id, message->from->id);
	int userIndex = getOrInsertUser(users, tmp);
	delete tmp;
	myUser* user = &(users->at(userIndex));

	switch (user->state){
		case NORMAL:
			if(message->chat->type==Chat::Type::Private) bot->getApi().sendMessage(user->chatId, "Non è il momento di scrivere cose a caso");
			break;

		case FEEDBACK:
			receivedFeedback(bot, user, message);
			user->state = NORMAL;
			break;
		
		case ANSWER:
			bot->getApi().sendMessage(answeringAt, message->text, false, answeringMessageId);
			bot->getApi().sendMessage(DEV_ID, "Risposta inviata con successo");
			user->state = NORMAL;
			break;

		default:
			break;
	}
}

void handleCallbackQuery(vector<Shotgun>* shotguns, vector<myUser>* users, TgBot::Bot* bot, TgBot::CallbackQuery::Ptr callback){
	myUser* tmp = new myUser(callback->message->chat->id, callback->from->id);
	int userIndex = getOrInsertUser(users, tmp);
	delete tmp;
	myUser* user = &(users->at(userIndex));

	if(StringTools::startsWith(callback->data, "answer")){
		handleAnswerQuery(shotguns, user, bot, callback);
	} else if(StringTools::startsWith(callback->data, "shotgun")){
		handleShotgunQuery(shotguns, user, bot, callback);
	} 
}

void handleAnswerQuery(vector<Shotgun>* shotguns, myUser* user, TgBot::Bot* bot, TgBot::CallbackQuery::Ptr callback){
	vector<string> options = StringTools::split(callback->data, ';');
	user->state = ANSWER;
	answeringMessageId = stoi(options[1]);
	answeringAt = stol(options[2]);
	bot->getApi().sendMessage(DEV_ID, "Scrivi la risposta al feedback o digita /cancel");
	bot->getApi().answerCallbackQuery(callback->id);
}

void handleShotgunQuery(vector<Shotgun>* shotguns, myUser* user, TgBot::Bot* bot, TgBot::CallbackQuery::Ptr callback){
	vector<string> options = StringTools::split(callback->data, ';');
	long _chatId = stol(options[2]);
	int _creatorId = stoi(options[3]);
	Shotgun* s = new Shotgun(_chatId, _creatorId);
	int shotgunIndex = findShotgun(shotguns, s);
	delete s;
	Shotgun* shotgun = NULL;

	if(options[1] == "stop"){
		if(shotgunIndex==-1){
			bot->getApi().deleteMessage(callback->message->chat->id, callback->message->messageId);
			bot->getApi().answerCallbackQuery(callback->id, "Errore, questo shotgun è già stato terminato", true);
			return;
		}
		shotgun = &(shotguns->at(shotgunIndex));

		if(user->id == shotgun->creatorId){
			shotgun->keyboard->inlineKeyboard.pop_back();

			bot->getApi().editMessageText(shotgun->messageText + "\n\n*Terminato*", shotgun->chatId, shotgun->messageId, "", "Markdown", false, shotgun->keyboard);
			if(callback->message->chat->type == Chat::Type::Group){
				try{bot->getApi().unpinChatMessage(user->chatId);}
				catch(exception& e){}
			}
			shotguns->erase(shotguns->begin()+shotgunIndex);
			bot->getApi().answerCallbackQuery(callback->id);
		} else {
			bot->getApi().answerCallbackQuery(callback->id, "Non puoi concludere uno shotgun che non hai creato!", true);
		}
		return;
	} else if(options[1] == "place"){
		if(shotgunIndex==-1){
			bot->getApi().answerCallbackQuery(callback->id, "Questo shotgun è già concluso!", true);
			return;
		}
		
		shotgun = &(shotguns->at(shotgunIndex));

		int i=stoi(options[4]);
		int j=stoi(options[5]);

		if(i == 0 && j == 0 && user->id != shotgun->creatorId){
			string m = "@" + callback->from->username + " tenta il colpo di stato e vuole guidare";
			bot->getApi().sendMessage(user->chatId, m);
			bot->getApi().answerCallbackQuery(callback->id);
			return;
		} else if (StringTools::startsWith(shotgun->keyboard->inlineKeyboard[i][j]->text, "@") 
			&& shotgun->keyboard->inlineKeyboard[i][j]->text != "@" + callback->from->username){
				bot->getApi().answerCallbackQuery(callback->id, "Questo posto è già occupato da " + shotgun->keyboard->inlineKeyboard[i][j]->text, true);
				return;
		}

		if(shotgun->keyboard->inlineKeyboard[i][j]->text == "@" + callback->from->username){
			shotgun->keyboard->inlineKeyboard[i][j]->text = "Libero";
			shotgun->keyboard->inlineKeyboard[i][j]->callbackData = "shotgun;place;" + to_string(shotgun->chatId) + ";" + to_string(shotgun->creatorId) + ";" + to_string(i) + ";" + to_string(j);
		} else {
			for(int n=0; n<shotgun->keyboard->inlineKeyboard.size(); n++){
				for(int m=0; m<shotgun->keyboard->inlineKeyboard[n].size(); m++){
					if(shotgun->keyboard->inlineKeyboard[n][m]->text == "@" + callback->from->username){
						shotgun->keyboard->inlineKeyboard[n][m]->text = "Libero";
						shotgun->keyboard->inlineKeyboard[n][m]->callbackData = "shotgun;place;" + to_string(shotgun->chatId) + ";" + to_string(shotgun->creatorId) + ";" + to_string(n) + ";" + to_string(m);
					}
				}
			}
			shotgun->keyboard->inlineKeyboard[i][j]->text = "@" + callback->from->username;
			shotgun->keyboard->inlineKeyboard[i][j]->callbackData = "shotgun;place;" + to_string(shotgun->chatId) + ";" + to_string(shotgun->creatorId) + ";" + to_string(i) + ";" + to_string(j);
		}

		bot->getApi().editMessageText(shotgun->messageText, shotgun->chatId, shotgun->messageId, "", "Markdown", false, shotgun->keyboard);
		bot->getApi().answerCallbackQuery(callback->id);
		return;
	}
}

void receivedFeedback(Bot* bot, myUser* user, Message::Ptr message){
	InlineKeyboardMarkup::Ptr kb(new InlineKeyboardMarkup());
	vector<InlineKeyboardButton::Ptr> row0;
	InlineKeyboardButton::Ptr btn(new InlineKeyboardButton());
	if(StringTools::startsWith(message->text, "/feedback")){
		message->text = message->text.substr(10);
	}

	string m = "Nuovo feedback ricevuto!\n" + message->text;
	btn->text = "Rispondi a @" + message->from->username;
	btn->callbackData = "answer;" + to_string(message->messageId) + ";" + to_string(user->chatId);
	row0.push_back(btn);
	kb->inlineKeyboard.push_back(row0);
	bot->getApi().sendMessage(user->chatId, "Feedback inviato con successo!");
	bot->getApi().sendMessage(DEV_ID, "Nuovo feedback ricevuto da @" + bot->getApi().getChatMember(user->chatId, user->id)->user->username + "\n\"_" + message->text + "_\"", false, 0, kb, "Markdown");
}