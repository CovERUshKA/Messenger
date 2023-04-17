import users
import database as db

users_chats = db.client["chats"]["users"]

groups_chats = db.client["chats"]["groups"]

notify = []

def get_chat(chat_id = None):
    if chat_id == None:
        return None

    chat = users_chats.find_one({"chat_id":chat_id})

    if len(chat) == 0:
        return None

    return chat[0]

def get_chat_with_users(first, second):
    user_ids = sorted([first, second])

    chat = users_chats.find_one({"users":user_ids}, {"_id":False, "users":True, "messages":True})

    if chat == None:
        return None

    return chat

def get_group_with_id(group_id):
    chat = groups_chats.find_one({"chat_id":group_id}, {"_id":False, "users":True, "messages":True, "name":True, "chat_id":True})

    if chat == None:
        return None

    return chat

def get_chats_with_user(user_id):
    user_chats = list(users_chats.find( { "users": user_id}, {"_id":False, "users":True, "messages":True}))

    group_chats = list(groups_chats.find( { "users": user_id}, {"_id":False, "users":True, "messages":True, "name":True, "chat_id":True}))

    for n, chat in enumerate(group_chats):
        users_with_info = []
        for group_user_id in chat["users"]:
            user = users.get_user(group_user_id)
            users_with_info.append({"user_id": user["user_id"], "username": user["username"]})
        group_chats[n]["users"] = users_with_info

    return user_chats + group_chats

def count_chats():
    return users_chats.count_documents({})

def count_groups():
    return groups_chats.count_documents({})
    
def add_message_to_chat(user_id, chat_id, data):
    chat : dict = get_chat_with_users(user_id, chat_id)
    
    if chat == None:
        return None
    
    user_ids = chat.get("users", None)
    
    messages = chat["messages"]
    
    count_messages = len(messages)
    
    data["message_id"] = count_messages + 1

    users_chats.update_one({"users":user_ids}, {"$push": {"messages":data}})

    data["chat"] = {}

    data["chat"]["chat_id"] = user_id
    data["chat"]["name"] = users.get_user_name(user_id)
    users.add_update(chat_id, {"message":data})

    data["chat"]["chat_id"] = chat_id
    data["chat"]["name"] = users.get_user_name(chat_id)
    users.add_update(user_id, {"message":data})

    for i in notify:
        if i[0] == user_id or i[0] == chat_id:
            i[1].set()
    
    return data

def add_message_to_group(user_id, chat_id, data):
    chat : dict = get_group_with_id(chat_id)
    
    if chat == None:
        return None
    
    user_ids = chat.get("users", None)
    
    messages = chat["messages"]
    
    count_messages = len(messages)
    
    data["message_id"] = count_messages + 1

    groups_chats.update_one({"chat_id":chat_id}, {"$push": {"messages":data}})

    data["chat"] = {}

    for user_id in user_ids:
        data["chat"]["chat_id"] = chat_id
        data["chat"]["name"] = chat["name"]
        users.add_update(user_id, {"message":data})

    for i in notify:
        if i[0] in user_ids:
            i[1].set()
    
    return data

# Create group
def create_group(name: str, creator_user_id: int, users_id: list):
    user = users.get_user(creator_user_id)
    if user == None:
        print(f"Unable to get creator user of group with id: {user_id}")
        return None
    
    for user_id in users_id:
        user = users.get_user(user_id)
        if user == None:
            print(f"Unable to get user with id: {user_id} to create group")
            return None
        
    users_id.append(creator_user_id)

    group_id = 0 - 1 - count_groups()

    groups_chats.insert_one({"chat_id": group_id, "name": name, "creator": creator_user_id, "users": sorted(users_id), "messages": []})

    chat = get_group_with_id(group_id)

    print(chat)

    users_with_info = []
    for group_user_id in chat["users"]:
        user = users.get_user(group_user_id)
        users_with_info.append({"user_id": user["user_id"], "username": user["username"]})
    chat["users"] = users_with_info

    if chat != None:
        return chat

    return None

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
