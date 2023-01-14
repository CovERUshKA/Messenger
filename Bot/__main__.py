import requests
import time

ip = "127.0.0.1"
bot_name = "test_bot"
password = "12345"

bot_user_id = None

s = requests.Session()

def send_message(chat_id, text):
    if bot_user_id == None:
        return False
    
    response = s.get(f"https://{ip}/api/v1/sendMessage?user_id={bot_user_id}&chat_id={chat_id}&text={text}", verify=False)

    return response.ok

def handle_message(message):
    chat = message.get("chat", None)

    text = message.get("text", None)
    chat_id = chat.get("chat_id", None)
    chat_name = chat.get("name", None)

    print(f"Получено сообщение \"{text}\" из чата {chat_name}")

    if text == "Hello":
        send_message(chat_id, "Hi")

def main():
    global bot_user_id

    while True:
        try:
            response = s.get(f"https://{ip}/api/v1/login?username={bot_name}&password={password}", verify=False)

            if response.ok:
                bot_data = response.json()["result"]
                bot_user_id = bot_data["user_id"]

                while True:
                    updates = s.get(f"https://{ip}/api/v1/getUpdates?user_id={bot_user_id}", verify=False)

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
        except requests.exceptions.ConnectionError:
            print("Не удалось установить соединение")
        
        time.sleep(1)
    
    return

if __name__ == "__main__":
    main()