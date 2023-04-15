import requests
import time
import random

ip = "127.0.0.1"
bot_name = "test_bot"
password = "12345"

bot_user_id = None
session_key = None

s = requests.Session()

def send_message(chat_id, text):
    if bot_user_id == None:
        return False
    
    headers = {
        "Authorization":f"Bearer {session_key}"
    }
    
    response = s.get(f"https://{ip}/api/v1/sendMessage?user_id={bot_user_id}&chat_id={chat_id}&text={text}", headers=headers, verify=False)

    return response.ok

def handle_message(message):
    chat = message.get("chat", None)

    text = message.get("text", None)
    chat_id = chat.get("chat_id", None)
    chat_name = chat.get("name", None)

    print(f"Получено сообщение \"{text}\" из чата {chat_name}")

    if text == "Hello":
        send_message(chat_id, "Hi")

    args = text.split(" ")

    if len(args) == 3:
        random_num = random.randint(int(args[1]), int(args[2]))
        send_message(chat_id, f"{random_num}")

def main():
    global bot_user_id
    global session_key

    while True:
        try:
            body = {
                "username": bot_name,
                "password": password
            }
            response = s.post(f"https://{ip}/api/v1/login", json=body, verify=False)

            if response.ok:
                bot_data = response.json()["result"]
                bot_user_id = bot_data["user_id"]
                session_key = bot_data["session_key"]

                headers = {
                    "Authorization": f"Bearer {session_key}"
                }

                while True:
                    updates = s.get(f"https://{ip}/api/v1/getUpdates", headers=headers, verify=False)

                    if updates.ok:
                        updates_json = updates.json()

                        for i in updates_json["result"]:
                            message = i.get("message", None)

                            if message != None:
                                handle_message(message)
                    else:
                        print("Unable to get updates")
                        print(updates.text)
                        time.sleep(1)
            else:
                print("Unable to login")
                print(response.text)
                body = {
                    "username": bot_name,
                    "password": password
                }
                s.post(f"https://{ip}/api/v1/createAccount", json=body, verify=False)
                
        except requests.exceptions.ConnectionError:
            print("Не удалось установить соединение")
        
        time.sleep(1)
    
    return

if __name__ == "__main__":
    main()
