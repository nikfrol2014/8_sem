import javax.swing.*;
import javax.swing.filechooser.FileNameExtensionFilter;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import javax.imageio.ImageIO;

public class LSBSteganographyApp extends JFrame {
    private JPanel mainPanel;
    private JButton loadImageButton;
    private JButton saveImageButton;
    private JButton encryptButton;
    private JButton decryptButton;
    private JTextArea messageTextArea;
    private JLabel imageLabel;
    private JLabel statusLabel;
    private JTextField maxMessageLengthField;
    private JLabel imageInfoLabel;
    private JComboBox<String> encodingComboBox;

    private BufferedImage originalImage;
    private BufferedImage modifiedImage;
    private File currentImageFile;
    private boolean isImageEncoded = false;

    // Маркеры для определения начала и конца сообщения (4 байта каждый)
    private static final byte[] START_MARKER = {(byte) 0xAA, (byte) 0xBB, (byte) 0xCC, (byte) 0xDD};
    private static final byte[] END_MARKER = {(byte) 0xEE, (byte) 0xFF, (byte) 0xAA, (byte) 0xBB};

    public LSBSteganographyApp() {
        setTitle("LSB Стеганография - скрытие текста в изображении");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(1200, 800);
        setLocationRelativeTo(null);

        initComponents();
        setupListeners();

        setVisible(true);
    }

    private void initComponents() {
        mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));

        // ========== ВЕРХНЯЯ ПАНЕЛЬ С КНОПКАМИ ==========
        JPanel topPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 10, 5));
        topPanel.setBorder(BorderFactory.createTitledBorder("Управление"));

        loadImageButton = new JButton("📁 Загрузить изображение");
        saveImageButton = new JButton("💾 Сохранить изображение");
        encryptButton = new JButton("🔒 Зашифровать сообщение");
        decryptButton = new JButton("🔓 Расшифровать сообщение");

        saveImageButton.setEnabled(false);
        encryptButton.setEnabled(false);
        decryptButton.setEnabled(false);

        // Делаем кнопки одинакового размера
        Dimension buttonSize = new Dimension(200, 35);
        loadImageButton.setPreferredSize(buttonSize);
        saveImageButton.setPreferredSize(buttonSize);
        encryptButton.setPreferredSize(buttonSize);
        decryptButton.setPreferredSize(buttonSize);

        // Добавляем подсказки к кнопкам
        loadImageButton.setToolTipText("Загрузить изображение для работы");
        saveImageButton.setToolTipText("Сохранить изображение со скрытым сообщением (только PNG!)");
        encryptButton.setToolTipText("Спрятать текст из правого поля в изображение");
        decryptButton.setToolTipText("Извлечь скрытый текст из изображения");

        topPanel.add(loadImageButton);
        topPanel.add(saveImageButton);
        topPanel.add(encryptButton);
        topPanel.add(decryptButton);

        // ========== ЦЕНТРАЛЬНАЯ ПАНЕЛЬ ==========
        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        splitPane.setResizeWeight(0.5);
        splitPane.setDividerSize(5);

        // Левая панель - изображение
        JPanel imagePanel = createImagePanel();

        // Правая панель - текст
        JPanel textPanel = createTextPanel();

        splitPane.setLeftComponent(imagePanel);
        splitPane.setRightComponent(textPanel);

        // ========== НИЖНЯЯ ПАНЕЛЬ (СТАТУС) ==========
        statusLabel = new JLabel("✅ Готов к работе. Загрузите изображение для начала.");
        statusLabel.setBorder(BorderFactory.createLoweredBevelBorder());
        statusLabel.setPreferredSize(new Dimension(0, 30));
        statusLabel.setFont(new Font("Arial", Font.PLAIN, 12));

        // Собираем всё вместе
        mainPanel.add(topPanel, BorderLayout.NORTH);
        mainPanel.add(splitPane, BorderLayout.CENTER);
        mainPanel.add(statusLabel, BorderLayout.SOUTH);

        add(mainPanel);
    }

    private JPanel createImagePanel() {
        JPanel panel = new JPanel(new BorderLayout(5, 5));
        panel.setBorder(BorderFactory.createTitledBorder("Изображение"));

        imageLabel = new JLabel();
        imageLabel.setHorizontalAlignment(JLabel.CENTER);
        imageLabel.setVerticalAlignment(JLabel.CENTER);
        imageLabel.setPreferredSize(new Dimension(500, 400));
        imageLabel.setBorder(BorderFactory.createLineBorder(Color.LIGHT_GRAY));

        JScrollPane imageScrollPane = new JScrollPane(imageLabel);
        imageScrollPane.setPreferredSize(new Dimension(550, 450));

        // Информация об изображении
        imageInfoLabel = new JLabel("Изображение не загружено");
        imageInfoLabel.setHorizontalAlignment(JLabel.CENTER);
        imageInfoLabel.setFont(new Font("Arial", Font.PLAIN, 12));

        panel.add(imageScrollPane, BorderLayout.CENTER);
        panel.add(imageInfoLabel, BorderLayout.SOUTH);

        return panel;
    }

    private JPanel createTextPanel() {
        JPanel panel = new JPanel(new BorderLayout(5, 5));
        panel.setBorder(BorderFactory.createTitledBorder("Текстовое сообщение"));

        // Верхняя панель с информацией
        JPanel topInfoPanel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets(5, 5, 5, 5);

        // Максимальная длина
        gbc.gridx = 0;
        gbc.gridy = 0;
        topInfoPanel.add(new JLabel("Макс. длина сообщения:"), gbc);

        gbc.gridx = 1;
        maxMessageLengthField = new JTextField(10);
        maxMessageLengthField.setEditable(false);
        maxMessageLengthField.setHorizontalAlignment(JTextField.CENTER);
        maxMessageLengthField.setFont(new Font("Monospaced", Font.BOLD, 14));
        topInfoPanel.add(maxMessageLengthField, gbc);

        gbc.gridx = 2;
        topInfoPanel.add(new JLabel("символов"), gbc);

        // Выбор кодировки
        gbc.gridx = 0;
        gbc.gridy = 1;
        topInfoPanel.add(new JLabel("Кодировка:"), gbc);

        gbc.gridx = 1;
        encodingComboBox = new JComboBox<>(new String[]{"UTF-8 (русский)", "Windows-1251", "ASCII"});
        encodingComboBox.setSelectedIndex(0);
        topInfoPanel.add(encodingComboBox, gbc);

        // Текстовая область
        messageTextArea = new JTextArea(15, 40);
        messageTextArea.setLineWrap(true);
        messageTextArea.setWrapStyleWord(true);
        messageTextArea.setFont(new Font("Monospaced", Font.PLAIN, 14));
        messageTextArea.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));

        JScrollPane messageScrollPane = new JScrollPane(messageTextArea);
        messageScrollPane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);

        // Панель с подсказкой и счетчиком символов
        JPanel bottomPanel = new JPanel(new BorderLayout());

        JLabel hintLabel = new JLabel("✏️ Введите текст для шифрования или здесь появится расшифрованное сообщение");
        hintLabel.setForeground(Color.GRAY);
        hintLabel.setFont(new Font("Arial", Font.ITALIC, 11));

        JLabel counterLabel = new JLabel("Символов: 0");
        counterLabel.setHorizontalAlignment(JLabel.RIGHT);
        counterLabel.setFont(new Font("Arial", Font.PLAIN, 11));

        // Добавляем слушатель для обновления счетчика
        messageTextArea.addKeyListener(new java.awt.event.KeyAdapter() {
            public void keyReleased(java.awt.event.KeyEvent evt) {
                counterLabel.setText("Символов: " + messageTextArea.getText().length());
            }
        });

        bottomPanel.add(hintLabel, BorderLayout.WEST);
        bottomPanel.add(counterLabel, BorderLayout.EAST);

        panel.add(topInfoPanel, BorderLayout.NORTH);
        panel.add(messageScrollPane, BorderLayout.CENTER);
        panel.add(bottomPanel, BorderLayout.SOUTH);

        return panel;
    }

    private void setupListeners() {
        loadImageButton.addActionListener(e -> loadImage());
        saveImageButton.addActionListener(e -> saveImage());
        encryptButton.addActionListener(e -> encryptMessage());
        decryptButton.addActionListener(e -> decryptMessage());
    }

    private void loadImage() {
        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setFileFilter(new FileNameExtensionFilter(
                "Изображения (*.png, *.bmp, *.jpg, *.jpeg)", "png", "bmp", "jpg", "jpeg"));

        if (fileChooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            try {
                currentImageFile = fileChooser.getSelectedFile();
                originalImage = ImageIO.read(currentImageFile);

                // Конвертируем в TYPE_INT_RGB для единообразия
                modifiedImage = convertToRGB(originalImage);
                originalImage = copyImage(modifiedImage);

                displayImage(modifiedImage);
                updateImageInfo();

                int maxMessageLength = calculateMaxMessageLength(modifiedImage);
                maxMessageLengthField.setText(String.valueOf(maxMessageLength));

                saveImageButton.setEnabled(true);
                encryptButton.setEnabled(true);
                decryptButton.setEnabled(true);
                isImageEncoded = false;

                statusLabel.setText("✅ Изображение загружено: " + currentImageFile.getName() +
                        " (" + originalImage.getWidth() + "x" + originalImage.getHeight() + ")");
                messageTextArea.setText("");
                messageTextArea.setEditable(true);

            } catch (IOException ex) {
                showError("Ошибка при загрузке изображения", ex.getMessage());
            }
        }
    }

    private BufferedImage convertToRGB(BufferedImage image) {
        BufferedImage rgbImage = new BufferedImage(image.getWidth(), image.getHeight(), BufferedImage.TYPE_INT_RGB);
        Graphics2D g = rgbImage.createGraphics();
        g.drawImage(image, 0, 0, null);
        g.dispose();
        return rgbImage;
    }

    private void displayImage(BufferedImage image) {
        if (image == null) return;

        int maxWidth = 500;
        int maxHeight = 400;

        int imgWidth = image.getWidth();
        int imgHeight = image.getHeight();

        double scale = Math.min((double) maxWidth / imgWidth, (double) maxHeight / imgHeight);
        int scaledWidth = (int) (imgWidth * scale);
        int scaledHeight = (int) (imgHeight * scale);

        Image scaledImage = image.getScaledInstance(scaledWidth, scaledHeight, Image.SCALE_SMOOTH);
        imageLabel.setIcon(new ImageIcon(scaledImage));
    }

    private void updateImageInfo() {
        if (originalImage != null) {
            String info = String.format("%d x %d пикселей, RGB",
                    originalImage.getWidth(), originalImage.getHeight());
            imageInfoLabel.setText(info);
        }
    }

    private int calculateMaxMessageLength(BufferedImage image) {
        // Каждый пиксель содержит 3 канала (RGB) по 1 биту = 3 бита на пиксель
        // Минус место для маркеров (8 байт = 64 бита)
        int totalBits = image.getWidth() * image.getHeight() * 3;
        int maxBytes = (totalBits / 8) - 16; // 16 байт для маркеров
        return Math.max(0, maxBytes);
    }

    private void saveImage() {
        if (modifiedImage == null) return;

        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setFileFilter(new FileNameExtensionFilter("PNG изображение (*.png)", "png"));
        fileChooser.setSelectedFile(new File("encoded_image.png"));

        if (fileChooser.showSaveDialog(this) == JFileChooser.APPROVE_OPTION) {
            try {
                File outputFile = fileChooser.getSelectedFile();
                if (!outputFile.getName().toLowerCase().endsWith(".png")) {
                    outputFile = new File(outputFile.getAbsolutePath() + ".png");
                }

                // Всегда сохраняем как PNG для сохранения данных
                ImageIO.write(modifiedImage, "png", outputFile);
                statusLabel.setText("✅ Изображение сохранено: " + outputFile.getName());
                showInfo("Успех", "Изображение успешно сохранено в формате PNG!");

            } catch (IOException ex) {
                showError("Ошибка при сохранении изображения", ex.getMessage());
            }
        }
    }

    private void encryptMessage() {
        String message = messageTextArea.getText();
        if (message.isEmpty()) {
            showWarning("Предупреждение", "Введите сообщение для шифрования");
            return;
        }

        try {
            // Выбираем кодировку
            String encoding = getEncoding();
            byte[] messageBytes = message.getBytes(encoding);

            // Проверяем длину
            int maxMessageLength = Integer.parseInt(maxMessageLengthField.getText());
            if (messageBytes.length > maxMessageLength) {
                showError("Ошибка", "Сообщение слишком длинное!\n" +
                        "Максимальный размер: " + maxMessageLength + " байт\n" +
                        "Текущий размер: " + messageBytes.length + " байт\n" +
                        "Используйте более короткое сообщение или большее изображение.");
                return;
            }

            modifiedImage = encodeMessage(originalImage, messageBytes);
            displayImage(modifiedImage);

            isImageEncoded = true;
            statusLabel.setText("✅ Сообщение зашифровано в изображении (кодировка: " + encoding + ")");
            showInfo("Успех", "Сообщение успешно зашифровано!\n" +
                    "Размер сообщения: " + messageBytes.length + " байт");

        } catch (Exception ex) {
            showError("Ошибка при шифровании", ex.getMessage());
            ex.printStackTrace();
        }
    }

    private void decryptMessage() {
        if (modifiedImage == null) return;

        try {
            byte[] decodedBytes = decodeMessage(modifiedImage);

            // Пробуем расшифровать в разных кодировках
            String message = tryDecodeWithEncodings(decodedBytes);

            messageTextArea.setText(message);
            statusLabel.setText("✅ Сообщение расшифровано (длина: " + decodedBytes.length + " байт)");

        } catch (Exception ex) {
            showError("Ошибка при расшифровке",
                    "Не удалось найти зашифрованное сообщение.\n" +
                            "Убедитесь, что:\n" +
                            "1. Изображение содержит скрытый текст\n" +
                            "2. Изображение сохранено в формате PNG\n" +
                            "3. Сообщение не было повреждено");
            ex.printStackTrace();
        }
    }

    private String getEncoding() {
        return switch (encodingComboBox.getSelectedIndex()) {
            case 0 -> "UTF-8";
            case 1 -> "Windows-1251";
            case 2 -> "ASCII";
            default -> "UTF-8";
        };
    }

    private String tryDecodeWithEncodings(byte[] bytes) {
        // Пробуем UTF-8
        try {
            return new String(bytes, StandardCharsets.UTF_8);
        } catch (Exception e) {
            // Пробуем Windows-1251
            try {
                return new String(bytes, "Windows-1251");
            } catch (Exception ex) {
                // Пробуем ASCII
                return new String(bytes, StandardCharsets.US_ASCII);
            }
        }
    }

    private BufferedImage copyImage(BufferedImage source) {
        BufferedImage copy = new BufferedImage(source.getWidth(), source.getHeight(), BufferedImage.TYPE_INT_RGB);
        Graphics2D g = copy.createGraphics();
        g.drawImage(source, 0, 0, null);
        g.dispose();
        return copy;
    }

    private BufferedImage encodeMessage(BufferedImage image, byte[] messageBytes) {
        BufferedImage encodedImage = copyImage(image);

        // Создаем полные данные для записи: маркер начала + данные + маркер конца
        byte[] fullData = new byte[START_MARKER.length + messageBytes.length + END_MARKER.length];
        System.arraycopy(START_MARKER, 0, fullData, 0, START_MARKER.length);
        System.arraycopy(messageBytes, 0, fullData, START_MARKER.length, messageBytes.length);
        System.arraycopy(END_MARKER, 0, fullData, START_MARKER.length + messageBytes.length, END_MARKER.length);

        int width = encodedImage.getWidth();
        int height = encodedImage.getHeight();

        int dataIndex = 0;
        int bitIndex = 0;

        // Проходим по всем пикселям
        outerLoop:
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int rgb = encodedImage.getRGB(x, y);

                int red = (rgb >> 16) & 0xFF;
                int green = (rgb >> 8) & 0xFF;
                int blue = rgb & 0xFF;

                // Изменяем младшие биты
                if (dataIndex < fullData.length) {
                    red = (red & 0xFE) | ((fullData[dataIndex] >> (7 - bitIndex)) & 1);
                    bitIndex++;

                    if (bitIndex == 8) {
                        bitIndex = 0;
                        dataIndex++;
                        if (dataIndex >= fullData.length) break outerLoop;
                    }
                }

                if (dataIndex < fullData.length) {
                    green = (green & 0xFE) | ((fullData[dataIndex] >> (7 - bitIndex)) & 1);
                    bitIndex++;

                    if (bitIndex == 8) {
                        bitIndex = 0;
                        dataIndex++;
                        if (dataIndex >= fullData.length) break outerLoop;
                    }
                }

                if (dataIndex < fullData.length) {
                    blue = (blue & 0xFE) | ((fullData[dataIndex] >> (7 - bitIndex)) & 1);
                    bitIndex++;

                    if (bitIndex == 8) {
                        bitIndex = 0;
                        dataIndex++;
                        if (dataIndex >= fullData.length) break outerLoop;
                    }
                }

                int newRgb = (red << 16) | (green << 8) | blue;
                encodedImage.setRGB(x, y, newRgb);
            }
        }

        return encodedImage;
    }

    private byte[] decodeMessage(BufferedImage image) {
        int width = image.getWidth();
        int height = image.getHeight();

        // Буфер для считывания данных
        java.util.ArrayList<Byte> dataList = new java.util.ArrayList<>();

        int currentByte = 0;
        int bitIndex = 0;

        // Читаем все младшие биты
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int rgb = image.getRGB(x, y);

                int red = (rgb >> 16) & 0xFF;
                int green = (rgb >> 8) & 0xFF;
                int blue = rgb & 0xFF;

                // Красный канал
                currentByte = (currentByte << 1) | (red & 1);
                bitIndex++;
                if (bitIndex == 8) {
                    dataList.add((byte) currentByte);
                    currentByte = 0;
                    bitIndex = 0;
                }

                // Зеленый канал
                currentByte = (currentByte << 1) | (green & 1);
                bitIndex++;
                if (bitIndex == 8) {
                    dataList.add((byte) currentByte);
                    currentByte = 0;
                    bitIndex = 0;
                }

                // Синий канал
                currentByte = (currentByte << 1) | (blue & 1);
                bitIndex++;
                if (bitIndex == 8) {
                    dataList.add((byte) currentByte);
                    currentByte = 0;
                    bitIndex = 0;
                }
            }
        }

        // Преобразуем в массив байт
        byte[] allBytes = new byte[dataList.size()];
        for (int i = 0; i < dataList.size(); i++) {
            allBytes[i] = dataList.get(i);
        }

        // Ищем маркеры
        return extractMessageBetweenMarkers(allBytes, START_MARKER, END_MARKER);
    }

    private byte[] extractMessageBetweenMarkers(byte[] data, byte[] startMarker, byte[] endMarker) {
        // Ищем начало
        int startPos = -1;
        for (int i = 0; i < data.length - startMarker.length; i++) {
            boolean found = true;
            for (int j = 0; j < startMarker.length; j++) {
                if (data[i + j] != startMarker[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                startPos = i + startMarker.length;
                break;
            }
        }

        final int messageLength = getMessageLength(data, endMarker, startPos);
        byte[] message = new byte[messageLength];
        System.arraycopy(data, startPos, message, 0, messageLength);

        return message;
    }

    private static int getMessageLength(byte[] data, byte[] endMarker, int startPos) {
        if (startPos == -1) {
            throw new RuntimeException("Не найден маркер начала сообщения");
        }

        // Ищем конец
        int endPos = -1;
        for (int i = startPos; i < data.length - endMarker.length; i++) {
            boolean found = true;
            for (int j = 0; j < endMarker.length; j++) {
                if (data[i + j] != endMarker[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                endPos = i;
                break;
            }
        }

        if (endPos == -1) {
            throw new RuntimeException("Не найден маркер конца сообщения");
        }

        // Извлекаем сообщение
        int messageLength = endPos - startPos;
        return messageLength;
    }

    private void showError(String title, String message) {
        JOptionPane.showMessageDialog(this, message, title, JOptionPane.ERROR_MESSAGE);
    }

    private void showWarning(String title, String message) {
        JOptionPane.showMessageDialog(this, message, title, JOptionPane.WARNING_MESSAGE);
    }

    private void showInfo(String title, String message) {
        JOptionPane.showMessageDialog(this, message, title, JOptionPane.INFORMATION_MESSAGE);
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            try {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } catch (Exception e) {
                e.printStackTrace();
            }
            new LSBSteganographyApp();
        });
    }
}