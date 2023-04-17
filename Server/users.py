import chats
import database as db
import argon2
import datetime
import jwt

from crypto import get_secret_key

ph = argon2.PasswordHasher()

auth_db = db.client.get_database("auth")

users = auth_db.get_collection("users")

def get_user(user_id):
    if type(user_id) != int:
        return None

    user = users.find_one({"user_id":user_id}, {"_id":False, "user_id":True, "username":True, "session_key":True, "updates":True})

    return user

def get_user_name(user_id):
    user = get_user(user_id)

    if user != None:
        return user["username"]
    
    return None

def get_user_with_username(username):
    user = users.find_one({"username": username}, {"_id":False, "user_id":True, "username":True, "password_hash":True, "session_key":True})

    return user

def login_user(username, password):
    user = get_user_with_username(username)

    if user == None:
        return False, "Account with this username doesn't exist"

    try:
        ph.verify(user["password_hash"], password)
    except argon2.exceptions.VerificationError:
        return False, "Incorrect password"
    
    user.pop("password_hash", None)

    payload = {
        "user_id": user["user_id"],
        'creation_time': datetime.datetime.utcnow().timestamp()
    }

    try:
        session_key = jwt.encode(payload,
               get_secret_key(),
               algorithm='HS256')
    except Exception as e:
        pass

    if session_key == None:
        return False, "Unable to create session key"
    
    user["session_key"] = session_key
    
    users.update_one({"user_id":user["user_id"]}, {"$set":{"session_key":session_key}})

    return True, user

def get_user_chats(user_id):
    user_chats = chats.get_chats_with_user(user_id)

    if user_chats == None:
        return []

    for chat in user_chats:
        print(chat)
        if chat.get("chat_id", None) != None:
            continue

        users = chat.get("users", None)
        if users == None:
            continue

        if users[0] == user_id:
            chat["chat_id"] = users[1]
            chat.pop("users")
        else:
            chat["chat_id"] = users[0]
            chat.pop("users")

        partner = get_user(chat["chat_id"])

        if partner != None:
            chat["name"] = partner["username"]

    return user_chats

def get_user_updates(user_id):
    user = get_user(user_id)

    if user == None:
        return None
    
    print(user)

    updates = user["updates"]

    if len(updates) == 0:
        return updates

    empty_user_updates(user_id)

    return updates

def add_update(user_id, update):
    user = get_user(user_id)

    if user == None:
        return None
    
    users.update_one({"user_id":user_id}, {"$push": {"updates": update}})

    return True

def empty_user_updates(user_id):
    user = get_user(user_id)

    if user == None:
        return None

    users.update_one({"user_id":user_id}, {"$set": {"updates": []}})

    return True

def search_users(data):
    users_list = list(users.find({"username": {"$regex":data}}, {"_id":False, "user_id":True, "username":True}))

    return users_list

def count_users():
    count = users.count_documents({})

    return count

def add_user(username, password):
    user = get_user_with_username(username)

    if user != None:
        return False

    user_id = count_users() + 1

    try:
        password_hash = ph.hash(password)
    except argon2.exceptions.HashingError:
        return False

    users.insert_one({"username": username, "password_hash": password_hash, "user_id": user_id, "updates":[], "session_key":None})
    
    success, user = login_user(username, password)

    return user
