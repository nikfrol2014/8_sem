#!/usr/bin/env python3.11
# -*- coding: utf-8 -*-

import os
import cv2
import numpy as np
from deepface import DeepFace
from PIL import Image, ImageTk
import tkinter as tk
from tkinter import filedialog, messagebox, Label, Button, Frame
from pathlib import Path
import threading

# Отключаем X Server ошибки
os.environ['OPENCV_IO_ENABLE_OPENEXR'] = '0'
os.environ['LIBGL_ALWAYS_SOFTWARE'] = '1'


class SimpleEmotionGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Определитель эмоций")
        self.root.geometry("1000x600")  # Скромный размер

        # Словарь эмоций
        self.emotions_ru = {
            'angry': 'Злость', 'disgust': 'Отвращение', 'fear': 'Страх',
            'happy': 'Счастье', 'sad': 'Грусть', 'surprise': 'Удивление',
            'neutral': 'Нейтрально'
        }

        self.create_widgets()
        self.image = None
        self.result_image = None

    def create_widgets(self):
        # Верхняя панель с кнопками (простая)
        top_frame = Frame(self.root)
        top_frame.pack(pady=10)

        Button(top_frame, text="Загрузить фото",
               command=self.load_image).pack(side=tk.LEFT, padx=5)

        Button(top_frame, text="Определить эмоции",
               command=self.detect_emotion).pack(side=tk.LEFT, padx=5)

        # Статус
        self.status = Label(self.root, text="Загрузите фотографию")
        self.status.pack(pady=5)

        # Изображение
        self.image_label = Label(self.root, bg="gray")
        self.image_label.pack(pady=10, padx=10, fill=tk.BOTH, expand=True)

        # Результат (простой текст)
        self.result_label = Label(self.root, text="", font=("Arial", 12))
        self.result_label.pack(pady=10)

    def load_image(self):
        file_path = filedialog.askopenfilename(
            filetypes=[("Images", "*.jpg *.jpeg *.png")]
        )

        if file_path:
            self.image = cv2.imread(file_path)
            self.image = cv2.cvtColor(self.image, cv2.COLOR_BGR2RGB)
            self.show_image(self.image)
            self.status.config(text=f"Загружено: {Path(file_path).name}")
            self.result_label.config(text="")

    def show_image(self, image):
        # Уменьшаем для отображения
        h, w = image.shape[:2]
        if h > 400 or w > 600:
            scale = min(400 / h, 600 / w)
            new_h, new_w = int(h * scale), int(w * scale)
            image = cv2.resize(image, (new_w, new_h))

        pil_image = Image.fromarray(image)
        photo = ImageTk.PhotoImage(pil_image)

        self.image_label.config(image=photo)
        self.image_label.image = photo

    def detect_emotion(self):
        if self.image is None:
            messagebox.showwarning("", "Сначала загрузите фото")
            return

        self.status.config(text="Анализ... (подождите)")
        self.root.update()

        # Запускаем в потоке
        thread = threading.Thread(target=self._analyze)
        thread.start()

    def _analyze(self):
        try:
            # Сохраняем временно
            temp_path = "/tmp/temp_emotion.jpg"
            cv2.imwrite(temp_path, cv2.cvtColor(self.image, cv2.COLOR_RGB2BGR))

            # Анализируем
            result = DeepFace.analyze(
                img_path=temp_path,
                actions=['emotion'],
                enforce_detection=False,
                silent=True
            )

            # Получаем результат
            if isinstance(result, list):
                face = result[0]
            else:
                face = result

            emotion = face['dominant_emotion']
            confidence = face['emotion'][emotion]
            ru_emotion = self.emotions_ru.get(emotion, emotion)

            # Показываем результат
            text = f"Эмоция: {ru_emotion} ({confidence:.1f}%)"

            # Добавляем детали
            details = "\n"
            for em, val in sorted(face['emotion'].items(), key=lambda x: x[1], reverse=True)[:3]:
                ru = self.emotions_ru.get(em, em)
                details += f"{ru}: {val:.1f}%  "

            self.root.after(0, self._update_result, text + details)

        except Exception as e:
            self.root.after(0, messagebox.showerror, "Ошибка", str(e))
        finally:
            self.root.after(0, lambda: self.status.config(text="Готов"))

    def _update_result(self, text):
        self.result_label.config(text=text)


if __name__ == "__main__":
    root = tk.Tk()
    app = SimpleEmotionGUI(root)
    root.mainloop()