import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.border.TitledBorder;
import java.awt.*;
import java.math.BigInteger;
import java.security.SecureRandom;
import java.util.HashMap;
import java.util.Map;

public class DiffieHellmanVariant18 extends JFrame {

    // Параметры варианта 18
    private static final BigInteger P = BigInteger.valueOf(13);  // простое число
    private static final BigInteger G = BigInteger.valueOf(53);  // примитивный корень

    private JTextField bobPrivateField;
    private JTextField alicePrivateField;
    private JTextArea resultArea;
    private JTextArea encryptionArea;
    private JLabel sharedSecretLabel;

    public DiffieHellmanVariant18() {
        setTitle("Алгоритм Диффи-Хеллмана - Вариант 18 (p=13, q=53)");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(900, 700);
        setLocationRelativeTo(null);

        initUI();
    }

    private void initUI() {
        // Главная панель
        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(new EmptyBorder(10, 10, 10, 10));

        // Верхняя панель с информацией о варианте
        mainPanel.add(createInfoPanel(), BorderLayout.NORTH);

        // Центральная панель
        JPanel centerPanel = new JPanel(new GridLayout(2, 1, 10, 10));
        centerPanel.add(createInputPanel());
        centerPanel.add(createResultPanel());
        mainPanel.add(centerPanel, BorderLayout.CENTER);

        // Нижняя панель
        mainPanel.add(createBottomPanel(), BorderLayout.SOUTH);

        // Панель шифрования справа
        mainPanel.add(createEncryptionPanel(), BorderLayout.EAST);

        add(mainPanel);

        // Устанавливаем начальные значения
        setDefaultValues();
    }

    private JPanel createInfoPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createEtchedBorder(),
                "Параметры варианта 18",
                TitledBorder.LEFT,
                TitledBorder.TOP,
                new Font("Arial", Font.BOLD, 16)
        ));

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(10, 10, 10, 10);
        gbc.fill = GridBagConstraints.HORIZONTAL;

        JLabel pLabel = new JLabel("P (простое число): 13");
        pLabel.setFont(new Font("Arial", Font.BOLD, 14));
        pLabel.setForeground(new Color(41, 128, 185));

        JLabel gLabel = new JLabel("G (примитивный корень): 53");
        gLabel.setFont(new Font("Arial", Font.BOLD, 14));
        gLabel.setForeground(new Color(41, 128, 185));

        gbc.gridx = 0; gbc.gridy = 0;
        panel.add(pLabel, gbc);

        gbc.gridx = 1;
        panel.add(gLabel, gbc);

        return panel;
    }

    private JPanel createInputPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createEtchedBorder(),
                "Ввод приватных ключей",
                TitledBorder.LEFT,
                TitledBorder.TOP,
                new Font("Arial", Font.BOLD, 14)
        ));

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(10, 10, 10, 10);
        gbc.fill = GridBagConstraints.HORIZONTAL;

        // Приватный ключ Боба
        gbc.gridx = 0; gbc.gridy = 0;
        JLabel bobLabel = new JLabel("Приватный ключ Боба:");
        bobLabel.setFont(new Font("Arial", Font.PLAIN, 12));
        panel.add(bobLabel, gbc);

        gbc.gridx = 1;
        bobPrivateField = new JTextField(15);
        bobPrivateField.setFont(new Font("Monospaced", Font.PLAIN, 14));
        panel.add(bobPrivateField, gbc);

        gbc.gridx = 2;
        JButton randomBobButton = new JButton("Случайный");
        randomBobButton.addActionListener(e -> generateRandomBobKey());
        panel.add(randomBobButton, gbc);

        // Приватный ключ Алисы
        gbc.gridx = 0; gbc.gridy = 1;
        JLabel aliceLabel = new JLabel("Приватный ключ Алисы:");
        aliceLabel.setFont(new Font("Arial", Font.PLAIN, 12));
        panel.add(aliceLabel, gbc);

        gbc.gridx = 1;
        alicePrivateField = new JTextField(15);
        alicePrivateField.setFont(new Font("Monospaced", Font.PLAIN, 14));
        panel.add(alicePrivateField, gbc);

        gbc.gridx = 2;
        JButton randomAliceButton = new JButton("Случайный");
        randomAliceButton.addActionListener(e -> generateRandomAliceKey());
        panel.add(randomAliceButton, gbc);

        // Кнопка генерации обоих ключей
        gbc.gridx = 0; gbc.gridy = 2;
        gbc.gridwidth = 3;
        JButton randomBothButton = new JButton("Сгенерировать оба случайных ключа");
        randomBothButton.setFont(new Font("Arial", Font.BOLD, 12));
        randomBothButton.setBackground(new Color(52, 152, 219));
        randomBothButton.setForeground(Color.WHITE);
        randomBothButton.addActionListener(e -> generateRandomKeys());
        panel.add(randomBothButton, gbc);

        return panel;
    }

    private JPanel createResultPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createEtchedBorder(),
                "Результаты вычислений",
                TitledBorder.LEFT,
                TitledBorder.TOP,
                new Font("Arial", Font.BOLD, 14)
        ));

        resultArea = new JTextArea();
        resultArea.setEditable(false);
        resultArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        resultArea.setBackground(new Color(250, 250, 250));
        resultArea.setBorder(new EmptyBorder(10, 10, 10, 10));

        JScrollPane scrollPane = new JScrollPane(resultArea);
        scrollPane.setPreferredSize(new Dimension(500, 200));
        panel.add(scrollPane, BorderLayout.CENTER);

        // Панель с общим секретом
        JPanel secretPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        secretPanel.setBorder(new EmptyBorder(5, 10, 5, 10));

        JLabel secretTitleLabel = new JLabel("Общий секретный ключ: ");
        secretTitleLabel.setFont(new Font("Arial", Font.BOLD, 14));

        sharedSecretLabel = new JLabel("не вычислен");
        sharedSecretLabel.setFont(new Font("Monospaced", Font.BOLD, 16));
        sharedSecretLabel.setForeground(new Color(192, 57, 43));

        secretPanel.add(secretTitleLabel);
        secretPanel.add(sharedSecretLabel);

        panel.add(secretPanel, BorderLayout.SOUTH);

        return panel;
    }

    private JPanel createEncryptionPanel() {
        JPanel panel = new JPanel(new BorderLayout(5, 5));
        panel.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createEtchedBorder(),
                "Шифрование сообщения",
                TitledBorder.LEFT,
                TitledBorder.TOP,
                new Font("Arial", Font.BOLD, 14)
        ));
        panel.setPreferredSize(new Dimension(300, 400));

        encryptionArea = new JTextArea();
        encryptionArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        encryptionArea.setLineWrap(true);
        encryptionArea.setWrapStyleWord(true);
        encryptionArea.setBorder(new EmptyBorder(5, 5, 5, 5));

        JScrollPane scrollPane = new JScrollPane(encryptionArea);
        scrollPane.setPreferredSize(new Dimension(280, 250));
        panel.add(scrollPane, BorderLayout.CENTER);

        JPanel buttonPanel = new JPanel(new GridLayout(3, 1, 5, 5));
        buttonPanel.setBorder(new EmptyBorder(5, 5, 5, 5));

        JButton encryptButton = createStyledButton("Зашифровать сообщение", new Color(46, 204, 113));
        JButton decryptButton = createStyledButton("Расшифровать сообщение", new Color(155, 89, 182));
        JButton clearButton = createStyledButton("Очистить", new Color(52, 73, 94));

        encryptButton.addActionListener(e -> encryptMessage());
        decryptButton.addActionListener(e -> decryptMessage());
        clearButton.addActionListener(e -> encryptionArea.setText(""));

        buttonPanel.add(encryptButton);
        buttonPanel.add(decryptButton);
        buttonPanel.add(clearButton);

        panel.add(buttonPanel, BorderLayout.SOUTH);

        return panel;
    }

    private JPanel createBottomPanel() {
        JPanel panel = new JPanel(new FlowLayout(FlowLayout.CENTER, 10, 10));

        JButton calculateButton = createStyledButton("Вычислить общий ключ", new Color(46, 204, 113));
        JButton demoButton = createStyledButton("Демо (как в примере)", new Color(52, 152, 219));
        JButton clearButton = createStyledButton("Очистить всё", new Color(231, 76, 60));

        calculateButton.addActionListener(e -> calculateSharedSecret());
        demoButton.addActionListener(e -> runDemo());
        clearButton.addActionListener(e -> clearAll());

        panel.add(calculateButton);
        panel.add(demoButton);
        panel.add(clearButton);

        return panel;
    }

    private JButton createStyledButton(String text, Color bgColor) {
        JButton button = new JButton(text);
        button.setFont(new Font("Arial", Font.BOLD, 12));
        button.setBackground(bgColor);
        button.setForeground(Color.WHITE);
        button.setFocusPainted(false);
        button.setBorder(BorderFactory.createRaisedBevelBorder());
        button.setPreferredSize(new Dimension(160, 40));

        button.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseEntered(java.awt.event.MouseEvent evt) {
                button.setBackground(bgColor.darker());
            }
            public void mouseExited(java.awt.event.MouseEvent evt) {
                button.setBackground(bgColor);
            }
        });

        return button;
    }

    private void setDefaultValues() {
        bobPrivateField.setText("5");
        alicePrivateField.setText("4");
    }

    private void generateRandomBobKey() {
        SecureRandom random = new SecureRandom();
        BigInteger bobPrivate = new BigInteger(P.bitLength() - 1, random);
        bobPrivateField.setText(bobPrivate.toString());
    }

    private void generateRandomAliceKey() {
        SecureRandom random = new SecureRandom();
        BigInteger alicePrivate = new BigInteger(P.bitLength() - 1, random);
        alicePrivateField.setText(alicePrivate.toString());
    }

    private void generateRandomKeys() {
        generateRandomBobKey();
        generateRandomAliceKey();
    }

    private void calculateSharedSecret() {
        try {
            BigInteger bobPrivate = new BigInteger(bobPrivateField.getText());
            BigInteger alicePrivate = new BigInteger(alicePrivateField.getText());

            // Проверка, что ключи в допустимом диапазоне
            if (bobPrivate.compareTo(BigInteger.ONE) < 0 || bobPrivate.compareTo(P) >= 0 ||
                    alicePrivate.compareTo(BigInteger.ONE) < 0 || alicePrivate.compareTo(P) >= 0) {
                JOptionPane.showMessageDialog(this,
                        "Приватные ключи должны быть в диапазоне (1, " + P + ")",
                        "Ошибка",
                        JOptionPane.ERROR_MESSAGE);
                return;
            }

            // Вычисление публичных ключей
            BigInteger publicBob = G.modPow(bobPrivate, P);
            BigInteger publicAlice = G.modPow(alicePrivate, P);

            // Вычисление общего секрета
            BigInteger secretBob = publicAlice.modPow(bobPrivate, P);
            BigInteger secretAlice = publicBob.modPow(alicePrivate, P);

            // Проверка совпадения
            if (!secretBob.equals(secretAlice)) {
                JOptionPane.showMessageDialog(this,
                        "Ошибка: ключи не совпадают!",
                        "Ошибка",
                        JOptionPane.ERROR_MESSAGE);
                return;
            }

            // Обновляем метку с общим секретом
            sharedSecretLabel.setText(secretBob.toString());
            sharedSecretLabel.setForeground(new Color(39, 174, 96));

            // Формирование подробного отчета
            StringBuilder report = new StringBuilder();
            report.append("╔════════════════════════════════════════════════════════════╗\n");
            report.append("║           АЛГОРИТМ ДИФФИ-ХЕЛЛМАНА (вариант 18)           ║\n");
            report.append("╚════════════════════════════════════════════════════════════╝\n\n");

            report.append("* ОТКРЫТЫЕ ПАРАМЕТРЫ:\n");
            report.append("   P = ").append(P).append(" (простое число)\n");
            report.append("   G = ").append(G).append(" (примитивный корень)\n\n");

            report.append("* ПРИВАТНЫЕ КЛЮЧИ (секретные):\n");
            report.append("   Приватный ключ Боба   = ").append(bobPrivate).append("\n");
            report.append("   Приватный ключ Алисы  = ").append(alicePrivate).append("\n\n");

            report.append("* ПУБЛИЧНЫЕ КЛЮЧИ (передаются открыто):\n");
            report.append("   Боб:   ").append(G).append("^").append(bobPrivate)
                    .append(" mod ").append(P).append(" = ").append(publicBob).append("\n");
            report.append("   Алиса: ").append(G).append("^").append(alicePrivate)
                    .append(" mod ").append(P).append(" = ").append(publicAlice).append("\n\n");

            report.append("* ВЫЧИСЛЕНИЕ ОБЩЕГО СЕКРЕТА:\n");
            report.append("   Боб:   ").append(publicAlice).append("^").append(bobPrivate)
                    .append(" mod ").append(P).append(" = ").append(secretBob).append("\n");
            report.append("   Алиса: ").append(publicBob).append("^").append(alicePrivate)
                    .append(" mod ").append(P).append(" = ").append(secretAlice).append("\n\n");

            report.append("* РЕЗУЛЬТАТ: Ключи совпадают!\n");
            report.append("   Общий секретный ключ: ").append(secretBob);

            resultArea.setText(report.toString());

        } catch (NumberFormatException ex) {
            JOptionPane.showMessageDialog(this,
                    "Пожалуйста, введите корректные числа",
                    "Ошибка",
                    JOptionPane.ERROR_MESSAGE);
        }
    }

    private void encryptMessage() {
        String secret = sharedSecretLabel.getText();
        if (secret.equals("не вычислен")) {
            JOptionPane.showMessageDialog(this,
                    "Сначала вычислите общий ключ",
                    "Ошибка",
                    JOptionPane.WARNING_MESSAGE);
            return;
        }

        String message = JOptionPane.showInputDialog(this,
                "Введите сообщение для шифрования:",
                "Шифрование",
                JOptionPane.PLAIN_MESSAGE);

        if (message != null && !message.isEmpty()) {
            try {
                // Используем ключ для шифрования (XOR для простоты)
                byte[] secretBytes = secret.getBytes();
                byte[] messageBytes = message.getBytes();
                byte[] encrypted = new byte[messageBytes.length];

                for (int i = 0; i < messageBytes.length; i++) {
                    encrypted[i] = (byte)(messageBytes[i] ^ secretBytes[i % secretBytes.length]);
                }

                StringBuilder result = new StringBuilder();
                result.append("*** ЗАШИФРОВАННОЕ СООБЩЕНИЕ\n");
                result.append("Исходное сообщение: ").append(message).append("\n\n");
                result.append("Hex: ");
                for (byte b : encrypted) {
                    result.append(String.format("%02X ", b));
                }
                result.append("\n\n");
                result.append("Base64: ").append(java.util.Base64.getEncoder().encodeToString(encrypted));

                encryptionArea.setText(result.toString());

            } catch (Exception ex) {
                JOptionPane.showMessageDialog(this,
                        "Ошибка при шифровании: " + ex.getMessage(),
                        "Ошибка",
                        JOptionPane.ERROR_MESSAGE);
            }
        }
    }

    private void decryptMessage() {
        String secret = sharedSecretLabel.getText();
        if (secret.equals("не вычислен")) {
            JOptionPane.showMessageDialog(this,
                    "Сначала вычислите общий ключ",
                    "Ошибка",
                    JOptionPane.WARNING_MESSAGE);
            return;
        }

        String encryptedText = encryptionArea.getText();
        if (encryptedText.isEmpty()) {
            JOptionPane.showMessageDialog(this,
                    "Нет зашифрованного сообщения",
                    "Ошибка",
                    JOptionPane.WARNING_MESSAGE);
            return;
        }

        try {
            // Извлекаем hex строку
            String[] lines = encryptedText.split("\n");
            String hexLine = "";
            for (String line : lines) {
                if (line.startsWith("Hex:")) {
                    hexLine = line.substring(4).trim();
                    break;
                }
            }

            if (hexLine.isEmpty()) {
                JOptionPane.showMessageDialog(this,
                        "Не удалось найти зашифрованные данные",
                        "Ошибка",
                        JOptionPane.ERROR_MESSAGE);
                return;
            }

            // Парсим hex
            String[] hexBytes = hexLine.split(" ");
            byte[] encrypted = new byte[hexBytes.length];
            for (int i = 0; i < hexBytes.length; i++) {
                encrypted[i] = (byte)Integer.parseInt(hexBytes[i], 16);
            }

            // Дешифрование
            byte[] secretBytes = secret.getBytes();
            byte[] decrypted = new byte[encrypted.length];

            for (int i = 0; i < encrypted.length; i++) {
                decrypted[i] = (byte)(encrypted[i] ^ secretBytes[i % secretBytes.length]);
            }

            encryptionArea.append("\n\n* РАСШИФРОВАННОЕ СООБЩЕНИЕ:\n");
            encryptionArea.append(new String(decrypted));

        } catch (Exception ex) {
            JOptionPane.showMessageDialog(this,
                    "Ошибка при дешифровании: " + ex.getMessage(),
                    "Ошибка",
                    JOptionPane.ERROR_MESSAGE);
        }
    }

    private void runDemo() {
        // Демонстрация как в примере из задания
        bobPrivateField.setText("5");
        alicePrivateField.setText("4");
        calculateSharedSecret();

        // Автоматически шифруем тестовое сообщение
        SwingUtilities.invokeLater(() -> {
            String message = "Привет, Алиса! Это секретное сообщение от Боба.";
            byte[] secretBytes = sharedSecretLabel.getText().getBytes();
            byte[] messageBytes = message.getBytes();
            byte[] encrypted = new byte[messageBytes.length];

            for (int i = 0; i < messageBytes.length; i++) {
                encrypted[i] = (byte)(messageBytes[i] ^ secretBytes[i % secretBytes.length]);
            }

            StringBuilder result = new StringBuilder();
            result.append("** ДЕМОНСТРАЦИОННОЕ СООБЩЕНИЕ\n");
            result.append("Исходное сообщение: ").append(message).append("\n\n");
            result.append("Hex: ");
            for (byte b : encrypted) {
                result.append(String.format("%02X ", b));
            }

            encryptionArea.setText(result.toString());
        });
    }

    private void clearAll() {
        bobPrivateField.setText("");
        alicePrivateField.setText("");
        resultArea.setText("");
        encryptionArea.setText("");
        sharedSecretLabel.setText("не вычислен");
        sharedSecretLabel.setForeground(new Color(192, 57, 43));
    }

    public static void main(String[] args) {
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e) {
            e.printStackTrace();
        }

        SwingUtilities.invokeLater(() -> {
            new DiffieHellmanVariant18().setVisible(true);
        });
    }
}