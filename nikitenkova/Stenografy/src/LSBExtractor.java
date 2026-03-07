import javax.swing.*;
import javax.swing.filechooser.FileNameExtensionFilter;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import javax.imageio.ImageIO;

public class LSBExtractor extends JFrame {
    private JPanel mainPanel;
    private JButton loadImageButton;
    private JButton saveTextButton;
    private JTextArea messageTextArea;
    private JLabel imageLabel;
    private JLabel statusLabel;
    private JLabel imageInfoLabel;
    private JTextArea hexDumpArea;
    private JLabel statsLabel;

    private BufferedImage currentImage;
    private byte[] extractedBytes;
    private String extractedText;

    public LSBExtractor() {
        setTitle("LSB Экстрактор - извлечение текста из изображения (2 бита)");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(1300, 850);
        setLocationRelativeTo(null);

        initComponents();
        setupListeners();

        setVisible(true);
    }

    private void initComponents() {
        mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));

        // ========== ВЕРХНЯЯ ПАНЕЛЬ ==========
        JPanel topPanel = new JPanel(new BorderLayout());

        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 20, 10));
        buttonPanel.setBorder(BorderFactory.createTitledBorder("Управление"));

        loadImageButton = new JButton(" 1. Загрузить изображение с сообщением");
        loadImageButton.setPreferredSize(new Dimension(300, 45));
        loadImageButton.setFont(new Font("Arial", Font.BOLD, 14));

        saveTextButton = new JButton(" 2. Сохранить извлеченный текст");
        saveTextButton.setPreferredSize(new Dimension(250, 45));
        saveTextButton.setFont(new Font("Arial", Font.BOLD, 14));
        saveTextButton.setEnabled(false);

        buttonPanel.add(loadImageButton);
        buttonPanel.add(saveTextButton);

        // Панель статистики
        JPanel statsPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        statsPanel.setBorder(BorderFactory.createEtchedBorder());
        statsLabel = new JLabel("Статистика: ожидание загрузки...");
        statsLabel.setFont(new Font("Monospaced", Font.PLAIN, 12));
        statsPanel.add(statsLabel);

        topPanel.add(buttonPanel, BorderLayout.NORTH);
        topPanel.add(statsPanel, BorderLayout.SOUTH);

        // ========== ЦЕНТРАЛЬНАЯ ПАНЕЛЬ (СПЛИТ) ==========
        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        splitPane.setResizeWeight(0.35);
        splitPane.setDividerSize(5);

        // Левая панель - изображение
        JPanel imagePanel = createImagePanel();

        // Правая панель - результат с вкладками
        JPanel resultPanel = createResultPanel();

        splitPane.setLeftComponent(imagePanel);
        splitPane.setRightComponent(resultPanel);

        // ========== НИЖНЯЯ ПАНЕЛЬ ==========
        statusLabel = new JLabel(" Загрузите изображение для извлечения текста");
        statusLabel.setBorder(BorderFactory.createLoweredBevelBorder());
        statusLabel.setPreferredSize(new Dimension(0, 30));
        statusLabel.setFont(new Font("Arial", Font.PLAIN, 12));

        mainPanel.add(topPanel, BorderLayout.NORTH);
        mainPanel.add(splitPane, BorderLayout.CENTER);
        mainPanel.add(statusLabel, BorderLayout.SOUTH);

        add(mainPanel);
    }

    private JPanel createImagePanel() {
        JPanel panel = new JPanel(new BorderLayout(5, 5));
        panel.setBorder(BorderFactory.createTitledBorder("Загруженное изображение"));

        imageLabel = new JLabel();
        imageLabel.setHorizontalAlignment(JLabel.CENTER);
        imageLabel.setVerticalAlignment(JLabel.CENTER);
        imageLabel.setPreferredSize(new Dimension(400, 450));
        imageLabel.setBorder(BorderFactory.createLineBorder(Color.LIGHT_GRAY));

        JScrollPane imageScrollPane = new JScrollPane(imageLabel);

        imageInfoLabel = new JLabel("Нет изображения");
        imageInfoLabel.setHorizontalAlignment(JLabel.CENTER);
        imageInfoLabel.setFont(new Font("Arial", Font.PLAIN, 12));

        panel.add(imageScrollPane, BorderLayout.CENTER);
        panel.add(imageInfoLabel, BorderLayout.SOUTH);

        return panel;
    }

    private JPanel createResultPanel() {
        JPanel panel = new JPanel(new BorderLayout(5, 5));
        panel.setBorder(BorderFactory.createTitledBorder("Результат извлечения"));

        // Создаем вкладки
        JTabbedPane tabbedPane = new JTabbedPane();

        // Вкладка с текстом
        JPanel textPanel = new JPanel(new BorderLayout());
        messageTextArea = new JTextArea();
        messageTextArea.setLineWrap(true);
        messageTextArea.setWrapStyleWord(true);
        messageTextArea.setFont(new Font("Monospaced", Font.PLAIN, 14));
        messageTextArea.setEditable(false);
        messageTextArea.setBackground(new Color(250, 250, 250));

        JScrollPane textScrollPane = new JScrollPane(messageTextArea);
        textPanel.add(textScrollPane, BorderLayout.CENTER);

        // Вкладка с HEX дампом
        JPanel hexPanel = new JPanel(new BorderLayout());
        hexDumpArea = new JTextArea();
        hexDumpArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        hexDumpArea.setEditable(false);
        hexDumpArea.setBackground(new Color(240, 240, 240));

        JScrollPane hexScrollPane = new JScrollPane(hexDumpArea);
        hexPanel.add(hexScrollPane, BorderLayout.CENTER);

        tabbedPane.addTab(" Извлеченный текст", textPanel);
        tabbedPane.addTab(" HEX дамп (сырые данные)", hexPanel);
        tabbedPane.addTab(" Информация", createInfoPanel());

        panel.add(tabbedPane, BorderLayout.CENTER);

        return panel;
    }

    private JPanel createInfoPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(10, 10, 10, 10);
        gbc.anchor = GridBagConstraints.WEST;
        gbc.gridx = 0;
        gbc.gridy = 0;

        String[] info = {
                "Метод извлечения: 2 младших бита (LSB) из RGB каналов",
                "Формат данных: Текст ASCII",
                "Маркер конца: нулевой байт (0x00)",
                "",
                "Процесс извлечения:",
                "1. Читаем пиксели изображения построчно",
                "2. Из каждого канала (R,G,B) берем 2 последних бита",
                "3. Из 8 бит собираем 1 байт",
                "4. Читаем байты до встречи нулевого байта (конец текста)",
                "",
                "Емкость изображения:",
                "Каждый пиксель дает 6 бит информации (2 бита × 3 канала)",
                "Для изображения 1000×1000: ~750 000 байт текста"
        };

        for (String line : info) {
            gbc.gridy++;
            panel.add(new JLabel(line), gbc);
        }

        return panel;
    }

    private void setupListeners() {
        loadImageButton.addActionListener(e -> loadImageAndExtract());
        saveTextButton.addActionListener(e -> saveTextToFile());
    }

    private void loadImageAndExtract() {
        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setFileFilter(new FileNameExtensionFilter(
                "Изображения (*.png, *.bmp, *.jpg)", "png", "bmp", "jpg", "jpeg"));
        fileChooser.setDialogTitle("Выберите изображение со скрытым текстом");

        if (fileChooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            try {
                File imageFile = fileChooser.getSelectedFile();
                statusLabel.setText(" Загрузка изображения: " + imageFile.getName());

                currentImage = ImageIO.read(imageFile);

                if (currentImage == null) {
                    throw new IOException("Не удалось загрузить изображение");
                }

                // Конвертируем в RGB
                BufferedImage rgbImage = new BufferedImage(
                        currentImage.getWidth(), currentImage.getHeight(),
                        BufferedImage.TYPE_INT_RGB);
                Graphics2D g = rgbImage.createGraphics();
                g.drawImage(currentImage, 0, 0, null);
                g.dispose();
                currentImage = rgbImage;

                // Отображаем изображение
                displayImage(currentImage);

                // Обновляем информацию
                updateImageInfo(currentImage);

                // ИЗВЛЕКАЕМ ТЕКСТ (основная функция)
                extractTextFromImage();

                statusLabel.setText(" Текст успешно извлечен из: " + imageFile.getName());
                saveTextButton.setEnabled(true);

            } catch (Exception ex) {
                showError("Ошибка при обработке изображения", ex.getMessage());
                statusLabel.setText(" Ошибка: " + ex.getMessage());
            }
        }
    }

    private void displayImage(BufferedImage image) {
        int maxWidth = 400;
        int maxHeight = 450;

        int imgWidth = image.getWidth();
        int imgHeight = image.getHeight();

        double scale = Math.min((double) maxWidth / imgWidth, (double) maxHeight / imgHeight);
        int scaledWidth = (int) (imgWidth * scale);
        int scaledHeight = (int) (imgHeight * scale);

        Image scaledImage = image.getScaledInstance(scaledWidth, scaledHeight, Image.SCALE_SMOOTH);
        imageLabel.setIcon(new ImageIcon(scaledImage));
    }

    private void updateImageInfo(BufferedImage image) {
        // Рассчитываем максимальную емкость при использовании 2 бит на канал
        int totalBits = image.getWidth() * image.getHeight() * 3 * 2; // 2 бита на канал
        int maxBytes = totalBits / 8;

        String info = String.format("%d × %d пикселей | Макс. емкость: %d байт (%.1f КБ)",
                image.getWidth(), image.getHeight(), maxBytes, maxBytes / 1024.0);
        imageInfoLabel.setText(info);
    }

    /**
     * ОСНОВНАЯ ФУНКЦИЯ ИЗВЛЕЧЕНИЯ ТЕКСТА
     * Использует 2 младших бита из каждого канала RGB
     * Читает до первого нулевого байта (маркер конца)
     */
    private void extractTextFromImage() {
        try {
            // Шаг 1: Извлекаем все байты из изображения
            extractedBytes = extractAllBytes2Bits(currentImage);

            // Шаг 2: Показываем HEX дамп
            showHexDump(extractedBytes);

            // Шаг 3: Ищем конец текста (нулевой байт)
            int textLength = findTextLength(extractedBytes);

            if (textLength == 0) {
                throw new RuntimeException("Не найден текст (первый байт уже нулевой)");
            }

            // Шаг 4: Извлекаем текст до нулевого байта
            byte[] textBytes = new byte[textLength];
            System.arraycopy(extractedBytes, 0, textBytes, 0, textLength);

            // Шаг 5: Конвертируем в ASCII строку
            extractedText = new String(textBytes, StandardCharsets.US_ASCII);

            // Шаг 6: Отображаем результат
            messageTextArea.setText(extractedText);
            messageTextArea.setCaretPosition(0);

            // Шаг 7: Обновляем статистику
            updateStatistics(textBytes.length, extractedBytes.length);

        } catch (Exception e) {
            messageTextArea.setText("ОШИБКА ИЗВЛЕЧЕНИЯ:\n" + e.getMessage());
            e.printStackTrace();
        }
    }

    /**
     * Извлекает все байты из изображения, используя 2 младших бита из каждого канала
     */
    private byte[] extractAllBytes2Bits(BufferedImage image) {
        ArrayList<Byte> bytesList = new ArrayList<>();

        int width = image.getWidth();
        int height = image.getHeight();

        int currentByte = 0;
        int bitCount = 0;

        // Проходим по всем пикселям
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int rgb = image.getRGB(x, y);

                // Получаем значения каналов
                int red = (rgb >> 16) & 0xFF;
                int green = (rgb >> 8) & 0xFF;
                int blue = rgb & 0xFF;

                // Извлекаем 2 младших бита из красного канала
                int redBits = red & 0x03; // 0x03 = 00000011 (2 бита)
                for (int b = 1; b >= 0; b--) { // Старший бит сначала
                    currentByte = (currentByte << 1) | ((redBits >> b) & 1);
                    bitCount++;

                    if (bitCount == 8) {
                        bytesList.add((byte) currentByte);
                        currentByte = 0;
                        bitCount = 0;
                    }
                }

                // Извлекаем 2 младших бита из зеленого канала
                int greenBits = green & 0x03;
                for (int b = 1; b >= 0; b--) {
                    currentByte = (currentByte << 1) | ((greenBits >> b) & 1);
                    bitCount++;

                    if (bitCount == 8) {
                        bytesList.add((byte) currentByte);
                        currentByte = 0;
                        bitCount = 0;
                    }
                }

                // Извлекаем 2 младших бита из синего канала
                int blueBits = blue & 0x03;
                for (int b = 1; b >= 0; b--) {
                    currentByte = (currentByte << 1) | ((blueBits >> b) & 1);
                    bitCount++;

                    if (bitCount == 8) {
                        bytesList.add((byte) currentByte);
                        currentByte = 0;
                        bitCount = 0;
                    }
                }
            }
        }

        // Добавляем последний байт, если он неполный
        if (bitCount > 0) {
            currentByte = currentByte << (8 - bitCount);
            bytesList.add((byte) currentByte);
        }

        // Конвертируем ArrayList в массив
        byte[] result = new byte[bytesList.size()];
        for (int i = 0; i < bytesList.size(); i++) {
            result[i] = bytesList.get(i);
        }

        return result;
    }

    /**
     * Находит длину текста до первого нулевого байта
     */
    private int findTextLength(byte[] bytes) {
        for (int i = 0; i < bytes.length; i++) {
            if (bytes[i] == 0) {
                return i; // Нашли нулевой байт - конец текста
            }
        }
        return bytes.length; // Нулевой байт не найден - используем все
    }

    /**
     * Показывает HEX дамп первых байт
     */
    private void showHexDump(byte[] bytes) {
        StringBuilder hex = new StringBuilder();
        StringBuilder ascii = new StringBuilder();

        int dumpSize = Math.min(512, bytes.length); // Показываем первые 512 байт

        hex.append("ИЗВЛЕЧЕННЫЕ ДАННЫЕ (первые ").append(dumpSize).append(" байт):\n");
        hex.append("Маркер конца: нулевой байт (0x00)\n");
        hex.append("----------------------------------------------------------------\n");
        hex.append("Offset  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | ASCII\n");
        hex.append("--------+------------------------------------------------+-----------------\n");

        for (int i = 0; i < dumpSize; i++) {
            if (i % 16 == 0) {
                if (i > 0) {
                    hex.append("| ").append(ascii).append("\n");
                    ascii = new StringBuilder();
                }
                hex.append(String.format("%06X | ", i));
            }

            byte b = bytes[i];
            hex.append(String.format("%02X ", b & 0xFF));

            // ASCII представление
            if (b >= 32 && b <= 126) {
                ascii.append((char) b);
            } else if (b == 0) {
                ascii.append('░'); // Специальный символ для нуля
            } else {
                ascii.append('·');
            }
        }

        // Дополняем последнюю строку
        int remaining = dumpSize % 16;
        if (remaining > 0) {
            for (int i = 0; i < 16 - remaining; i++) {
                hex.append("   ");
            }
        }
        hex.append("| ").append(ascii).append("\n");

        // Добавляем информацию о нулевом байте
        int zeroPos = findTextLength(bytes);
        hex.append("\n");
        hex.append("────────────────────────────────────────────────────────────────\n");
        hex.append("📍 Первый нулевой байт (конец текста) на позиции: ").append(zeroPos).append("\n");
        if (zeroPos < bytes.length) {
            hex.append(" Текст занимает ").append(zeroPos).append(" байт\n");
            hex.append(" Всего извлечено байт: ").append(bytes.length).append("\n");
        } else {
            hex.append(" Нулевой байт не найден! Использованы все ").append(bytes.length).append(" байт\n");
        }

        hexDumpArea.setText(hex.toString());
    }

    /**
     * Обновляет статистику
     */
    private void updateStatistics(int textBytes, int totalBytes) {
        StringBuilder stats = new StringBuilder();
        stats.append(" СТАТИСТИКА: ");
        stats.append("Текст: ").append(textBytes).append(" байт");
        stats.append(" | Всего данных: ").append(totalBytes).append(" байт");
        stats.append(" | Размер изображения: ");
        stats.append(currentImage.getWidth()).append("×").append(currentImage.getHeight());

        // Добавляем информацию о повторениях
        stats.append(" | Первые 20 символов: \"");
        if (extractedText != null && extractedText.length() > 0) {
            String preview = extractedText.substring(0, Math.min(20, extractedText.length()));
            stats.append(preview).append("\"");
        }

        statsLabel.setText(stats.toString());
    }

    /**
     * Сохраняет извлеченный текст в файл
     */
    private void saveTextToFile() {
        if (extractedText == null || extractedText.isEmpty()) {
            showError("Ошибка", "Нет текста для сохранения");
            return;
        }

        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setSelectedFile(new File("extracted_text.txt"));
        fileChooser.setFileFilter(new FileNameExtensionFilter("Текстовые файлы (*.txt)", "txt"));

        if (fileChooser.showSaveDialog(this) == JFileChooser.APPROVE_OPTION) {
            try {
                File outputFile = fileChooser.getSelectedFile();
                if (!outputFile.getName().toLowerCase().endsWith(".txt")) {
                    outputFile = new File(outputFile.getAbsolutePath() + ".txt");
                }

                // Сохраняем в ASCII
                try (FileOutputStream fos = new FileOutputStream(outputFile);
                     OutputStreamWriter osw = new OutputStreamWriter(fos, StandardCharsets.US_ASCII)) {
                    osw.write(extractedText);
                }

                JOptionPane.showMessageDialog(this,
                        "Текст сохранен в файл:\n" + outputFile.getAbsolutePath(),
                        "Успех", JOptionPane.INFORMATION_MESSAGE);

                statusLabel.setText(" Текст сохранен в: " + outputFile.getName());

            } catch (IOException ex) {
                showError("Ошибка сохранения", ex.getMessage());
            }
        }
    }

    private void showError(String title, String message) {
        JOptionPane.showMessageDialog(this, message, title, JOptionPane.ERROR_MESSAGE);
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            try {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } catch (Exception e) {
                e.printStackTrace();
            }
            new LSBExtractor();
        });
    }
}