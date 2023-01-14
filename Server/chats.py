import users
import database as db

from tinydb import Query

chats = db.chats

#first = chats.update("1")

Chat = Query()

lock = db.lock

notify = []

def get_chat(chat_id = None):
    if chat_id == None:
        return None

    try:
        lock.acquire(True)

        chat = chats.search(Chat.chat_id == chat_id)
    finally:
        lock.release()

    if len(chat) == 0:
        return None

    return chat[0]

def get_chats_with_user(user_id):
    try:
        lock.acquire(True)

        user_chats = chats.search((Chat.first == user_id) | (Chat.second == user_id))
    finally:
        lock.release()

    if len(user_chats) == 0:
        return None

    return user_chats

def get_chat_with_users(first, second):
    try:
        lock.acquire(True)

        chat = chats.search(((Chat.first == first) & (Chat.second == second)) | ((Chat.first == second) & (Chat.second == first)))
    finally:
        lock.release()

    if len(chat) == 0:
        return None

    return chat[0]

def count_chats():
    return len(chats.all())
    
def add_message_to_chat(first, second, data):
    chat = get_chat_with_users(first, second)
    
    if chat == None:
        return None
    
    messages = chat["messages"]
    
    count_messages = len(messages)
    
    data["message_id"] = count_messages + 1
    
    chat["messages"][count_messages + 1] = data

    try:
        lock.acquire(True)

        ret = chats.update(chat, ((Chat.first == first) & (Chat.second == second)) | ((Chat.first == second) & (Chat.second == first)))
    finally:
        lock.release()

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
        print("Chat not equal zero")
        return None

    user = users.get_user(first)
    if user == None:
        print("Unable to get first user")
        return None

    user = users.get_user(second)
    if user == None:
        print("Unable to get second user")
        return None

    chats.insert({"first": first, "second": second, "messages": {}})

    chat = get_chat_with_users(first, second)

    if chat != None:
        return chat

    return None