#!/usr/bin/env python3.11
# -*- coding: utf-8 -*-

import os
import face_recognition
import numpy as np
import tkinter as tk
from tkinter import filedialog, messagebox, Label, Button, Frame
from PIL import Image, ImageTk
import threading
import warnings

os.environ['OPENCV_IO_ENABLE_OPENEXR'] = '0'
warnings.filterwarnings("ignore")


class FaceCompare:
    def __init__(self, root):
        self.root = root
        self.root.title("Сравнение лиц")
        self.root.geometry("1000x650")

        self.img1 = None
        self.img2 = None
        self.is_comparing = False

        self.create_widgets()

    def create_widgets(self):
        # Верхняя панель
        top_frame = Frame(self.root)
        top_frame.pack(pady=5)

        Button(top_frame, text="Загрузить лицо 1", command=lambda: self.load(1)).pack(side=tk.LEFT, padx=5)
        Button(top_frame, text="Загрузить лицо 2", command=lambda: self.load(2)).pack(side=tk.LEFT, padx=5)
        Button(top_frame, text="Сравнить", command=self.compare).pack(side=tk.LEFT, padx=5)
        Button(top_frame, text="Очистить", command=self.clear).pack(side=tk.LEFT, padx=5)

        # Выбор бэкенда
        backend_frame = Frame(self.root)
        backend_frame.pack(pady=5)

        Label(backend_frame, text="Бэкенд:").pack(side=tk.LEFT)

        self.backend_var = tk.StringVar(value="hog")
        tk.Radiobutton(backend_frame, text="HOG (быстро)",
                       variable=self.backend_var, value="hog").pack(side=tk.LEFT, padx=5)
        tk.Radiobutton(backend_frame, text="CNN (точно)",
                       variable=self.backend_var, value="cnn").pack(side=tk.LEFT, padx=5)

        # Изображения
        image_frame = Frame(self.root)
        image_frame.pack(pady=10, fill=tk.BOTH, expand=True)

        self.label1 = Label(image_frame, bg="gray")
        self.label1.pack(side=tk.LEFT, padx=10, fill=tk.BOTH, expand=True)

        self.label2 = Label(image_frame, bg="gray")
        self.label2.pack(side=tk.RIGHT, padx=10, fill=tk.BOTH, expand=True)

        # Панель результатов
        result_frame = Frame(self.root, relief=tk.SUNKEN, borderwidth=2)
        result_frame.pack(fill=tk.X, padx=10, pady=10)

        Label(result_frame, text="РЕЗУЛЬТАТЫ СРАВНЕНИЯ", font=("Arial", 12, "bold")).pack()

        self.result_label = Label(result_frame, text="", font=("Arial", 11),
                                  justify=tk.LEFT, height=6)
        self.result_label.pack(pady=5, padx=10, fill=tk.X)

        # Статус
        self.status = Label(self.root, text="Готов к работе", fg="blue")
        self.status.pack()

    def load(self, num):
        path = filedialog.askopenfilename(filetypes=[("Images", "*.jpg *.jpeg *.png")])
        if path:
            img = face_recognition.load_image_file(path)
            pil_img = Image.fromarray(img)
            pil_img.thumbnail((300, 300))
            photo = ImageTk.PhotoImage(pil_img)

            if num == 1:
                self.img1 = img
                self.label1.config(image=photo)
                self.label1.image = photo
            else:
                self.img2 = img
                self.label2.config(image=photo)
                self.label2.image = photo

            self.status.config(text=f"Загружено лицо {num}")

    def clear(self):
        self.img1 = None
        self.img2 = None
        self.label1.config(image="")
        self.label2.config(image="")
        self.result_label.config(text="")
        self.status.config(text="Очищено")

    def compare(self):
        if self.img1 is None or self.img2 is None:
            messagebox.showwarning("", "Загрузите оба изображения")
            return

        if self.is_comparing:
            return

        self.is_comparing = True
        self.result_label.config(text="СРАВНЕНИЕ...")
        self.status.config(text="Сравнение...")
        self.root.update()

        thread = threading.Thread(target=self._compare_thread)
        thread.daemon = True
        thread.start()

    def _compare_thread(self):
        try:
            backend = self.backend_var.get()

            # Поиск лиц
            locs1 = face_recognition.face_locations(self.img1, model=backend)
            locs2 = face_recognition.face_locations(self.img2, model=backend)

            if not locs1 or not locs2:
                self._update_result("ЛИЦА НЕ НАЙДЕНЫ на одном из изображений")
                return

            # Эмбеддинги
            enc1 = face_recognition.face_encodings(self.img1, [locs1[0]])[0]
            enc2 = face_recognition.face_encodings(self.img2, [locs2[0]])[0]

            # Евклидово расстояние
            euclidean = np.linalg.norm(enc1 - enc2)

            # Косинусное сходство
            dot = np.dot(enc1, enc2)
            norm1 = np.sqrt(np.sum(enc1 ** 2))
            norm2 = np.sqrt(np.sum(enc2 ** 2))
            cosine = dot / (norm1 * norm2 + 1e-10)

            # Формируем результат (БЕЗ ЭМОДЗИ!)
            result = []
            result.append(f"Бэкенд: {backend.upper()}")
            result.append("-" * 40)
            result.append(f"Евклидово расстояние: {euclidean:.4f}")
            result.append(f"Косинусное сходство: {cosine:.4f}")
            result.append("-" * 40)

            # Интерпретация
            if euclidean < 0.6:
                result.append("ПО ЕВКЛИДОВУ: лица совпадают")
            else:
                result.append("ПО ЕВКЛИДОВУ: лица разные")

            if cosine > 0.7:
                result.append("ПО КОСИНУСУ: лица совпадают")
            else:
                result.append("ПО КОСИНУСУ: лица разные")

            # Общий вердикт
            result.append("-" * 40)
            if euclidean < 0.6 and cosine > 0.7:
                result.append("ИТОГ: ЛИЦА СОВПАДАЮТ")
            elif euclidean < 0.6 or cosine > 0.7:
                result.append("ИТОГ: ВОЗМОЖНО СОВПАДАЮТ (проверьте)")
            else:
                result.append("ИТОГ: ЛИЦА РАЗНЫЕ")

            self._update_result("\n".join(result))

        except Exception as e:
            self._update_result(f"ОШИБКА: {str(e)}")
        finally:
            self.is_comparing = False

    def _update_result(self, text):
        self.root.after(0, lambda: self.result_label.config(text=text))
        self.root.after(0, lambda: self.status.config(text="Сравнение завершено"))


if __name__ == "__main__":
    root = tk.Tk()
    app = FaceCompare(root)
    root.mainloop()