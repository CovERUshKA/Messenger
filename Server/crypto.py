import os
import base64
import database as db

server_database = db.client.get_database("server")
settings_collection = server_database.get_collection("settings")

def generate_random_string(len:int):
    salt = os.urandom(len)
    salt = str(base64.b64encode(salt))
    return salt[2:-1]

def get_secret_key():
    settings = settings_collection.find_one({})

    if settings == None:
        secret_key = generate_random_string(32)
        document = {
            "secret_key": secret_key
        }
        settings_collection.insert_one(document)
    else:
        secret_key = settings.get("secret_key", None)

        if secret_key == None:
            secret_key = generate_random_string(32)
            settings_collection.update_one({}, {"$set":{"secret_key":secret_key}})

    return secret_key