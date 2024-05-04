import speech_recognition as sr
import os
from openai import OpenAI
import serial
import time


def listen_and_convert():
    r = sr.Recognizer()
    with sr.Microphone() as source:
        print("Say something!")
        audio = r.listen(source)

    try:
        return r.recognize_google(audio)
    except sr.UnknownValueError:
        print("Google Speech Recognition could not understand audio")
        return None
    except sr.RequestError as e:
        print(f"{e}")
        return None

mode = 'voice'

if __name__ == "__main__":
    if mode == 'voice':
        text = listen_and_convert()
        if text:
            print(f"Google Speech Recognition thinks you said: {text}")
        else:
            print("Failed to convert audio to text")
        instruction = text
    else:
        instruction = 'Draw a letter b'

    key = os.getenv("OPENAI_API_KEY")
    # print(key)
    client = OpenAI(api_key=key)

    completion = client.chat.completions.create(
        model="gpt-4-turbo",
        messages=[
            {
                "role": "system",
                "content": "You are a assistant who translates natural language to commands of the form (direction, seconds). \
                Direction can only be 'left, right, straight'\
                Seconds can be 0-255ms.\
                Natural languages can be symbolic like 'draw a letter b', in that case provide commands for that,\
                When turning left or right there is turning radius so consider that,\
                Approximately turn left for 160ms = 90 degrees with 1 meter radius, straight for 160ms = go straight 1 meter,\
                Give a fixed number of four commands to fullfill the need.\
                For example: straight, 100, right, 200, left, 200, straight, 100\
                time cannot exceed 256ms\
                Only output commands but nothing else.",
            },
            {"role": "user", "content": f"Translate this: {instruction}"},
        ],
    )
    answer = completion.choices[0].message.content
    commands = answer.split(',')
    print(commands)

    # Setup the serial connection
    ser = serial.Serial('/dev/tty.DSDTECHHC-05', 9600)

    # Mapping of textual commands to byte values
    direction_map = {
        "left": 1,
        "right": 2,
        "straight": 0
    }

    try:
        data_packet = []

        for i, command in enumerate(commands):            
            if i % 2 == 0:
                try: 
                    direction = direction_map[command.strip()]
                except KeyError:
                    direction = 0
                data_packet.append(direction)
            else:
                duration = int(command)
                data_packet.append(duration)

        print(data_packet)
        to_send = [0 for _ in range(8)]
        to_send[0] = data_packet[0]
        to_send[1] = data_packet[2]
        to_send[2] = data_packet[4]
        to_send[3] = data_packet[6]
        to_send[4] = data_packet[1]
        to_send[5] = data_packet[3]
        to_send[6] = data_packet[5]
        to_send[7] = data_packet[7]
        print(to_send)
        to_send = bytearray(to_send)
        # Send the packet
        print(f"Sending: {to_send}")
        for _ in range(5):
            ser.write(to_send)
            time.sleep(.5)
    finally:
        ser.close() 