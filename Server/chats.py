import users
import database as db

users_chats = db.client["chats"]["users"]

#first = chats.update("1")

notify = []

def get_chat(chat_id = None):
    if chat_id == None:
        return None

    chat = users_chats.find_one({"chat_id":chat_id})

    if len(chat) == 0:
        return None

    return chat[0]

def get_chats_with_user(user_id):
    user_chats = list(users_chats.find( { "users": user_id}, {"_id":False, "users":True, "messages":True}))

    if len(user_chats) == 0:
        return None

    return user_chats

def get_chat_with_users(first, second):
    user_ids = sorted([first, second])

    chat = list(users_chats.find({"users":user_ids}, {"_id":False, "users":True, "messages":True}))

    if len(chat) == 0:
        return None

    return chat[0]

def count_chats():
    return len(users_chats.count_documents({}))
    
def add_message_to_chat(first, second, data):
    chat : dict = get_chat_with_users(first, second)
    
    if chat == None:
        return None
    
    user_ids = chat.get("users", None)
    
    messages = chat["messages"]
    
    count_messages = len(messages)
    
    data["message_id"] = count_messages + 1
    
    #chat["messages"][count_messages + 1] = data

    users_chats.update_one({"users":user_ids}, {"$push": {"messages":data}})

    data["chat"] = {}

    data["chat"]["chat_id"] = first
    data["chat"]["name"] = users.get_user_name(first)
    users.add_update(second, {"message":data})

    data["chat"]["chat_id"] = second
    data["chat"]["name"] = users.get_user_name(second)
    users.add_update(first, {"message":data})

    for i in notify:
        if i[0] == first or i[0] == second:
            i[1].set()
    
    return data

def add_chat(first, second):
    chat = get_chat_with_users(first, second)

    if chat != None:
        print("Chat already exists")
        return None

    user = users.get_user(first)
    if user == None:
        print("Unable to get first user")
        return None

    user = users.get_user(second)
    if user == None:
        print("Unable to get second user")
        return None

    users_chats.insert_one({"users": sorted([first, second]), "messages": []})

    chat = get_chat_with_users(first, second)

    if chat != None:
        return chat

    return None
