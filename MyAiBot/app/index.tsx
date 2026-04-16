import React, { useRef, useState, useEffect } from 'react';
import {
  ActivityIndicator,
  FlatList,
  KeyboardAvoidingView,
  Platform,
  SafeAreaView,
  StatusBar,
  StyleSheet,
  Text,
  TextInput,
  TouchableOpacity,
  View,
  Animated,
  Vibration
} from 'react-native';

interface Message {
  id: string;
  text: string;
  sender: 'user' | 'bot';
}

const FadeInMessage = ({ children }: { children: React.ReactNode }) => {
  const fadeAnim = useRef(new Animated.Value(0)).current;
  const slideAnim = useRef(new Animated.Value(10)).current;

  useEffect(() => {
    Animated.parallel([
      Animated.timing(fadeAnim, { toValue: 1, duration: 400, useNativeDriver: true }),
      Animated.timing(slideAnim, { toValue: 0, duration: 400, useNativeDriver: true }),
    ]).start();
  }, []);

  return (
    <Animated.View style={{ opacity: fadeAnim, transform: [{ translateY: slideAnim }] }}>
      {children}
    </Animated.View>
  );
};

export default function ChatScreen() {
  const [inputText, setInputText] = useState('');
  const [loading, setLoading] = useState(false);
  const [chatHistory, setChatHistory] = useState<Message[]>([
    { id: '1', text: 'Salam Azlan! Restock AI Pro is online and connected to the cloud.', sender: 'bot' }
  ]);
  
  const flatListRef = useRef<FlatList>(null);
  const isInputEmpty = inputText.trim().length === 0;

  const sendMessage = async () => {
    if (isInputEmpty) return;

    Vibration.vibrate(10); 

    const userMsg: Message = { id: Date.now().toString(), text: inputText, sender: 'user' };
    const updatedHistory = [...chatHistory, userMsg];
    setChatHistory(updatedHistory);
    setInputText('');
    setLoading(true);

    try {
      // --- UPDATED TO RENDER URL ---
      const response = await fetch('https://restock-python-backend.onrender.com/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message: userMsg.text }),
      });

      if (!response.ok) throw new Error('Server issues');

      const data = await response.json();
      
      Vibration.vibrate(20); 

      setChatHistory([...updatedHistory, { id: (Date.now() + 1).toString(), text: data.reply, sender: 'bot' }]);
    } catch (error) {
      // Friendly error message for "cold starts" on Render
      setChatHistory([...updatedHistory, { 
        id: 'err', 
        text: 'The engine is warming up (Render free tier). Please try sending that again in 30 seconds!', 
        sender: 'bot' 
      }]);
    } finally {
      setLoading(false);
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="dark-content" />
      
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Restock AI</Text>
      </View>

      <KeyboardAvoidingView 
        behavior={Platform.OS === 'ios' ? 'padding' : 'height'} 
        style={{ flex: 1 }}
      >
        <FlatList
          ref={flatListRef}
          data={chatHistory}
          keyExtractor={(item) => item.id}
          onContentSizeChange={() => flatListRef.current?.scrollToEnd({ animated: true })}
          renderItem={({ item }) => (
            <FadeInMessage>
              <View style={styles.messageRow}>
                <Text style={styles.label}>{item.sender === 'user' ? 'Azlan' : 'Restock AI'}</Text>
                <Text style={styles.messageText}>{item.text}</Text>
              </View>
            </FadeInMessage>
          )}
          contentContainerStyle={styles.listContent}
        />

        {loading && (
          <View style={styles.typingContainer}>
            <Text style={styles.typingText}>Engine is thinking...</Text>
          </View>
        )}

        <View style={styles.footer}>
          <View style={styles.inputWrapper}>
            <TextInput
              style={styles.input}
              value={inputText}
              onChangeText={setInputText}
              placeholder="Ask anything..."
              placeholderTextColor="#A1A1A1"
            />
            <TouchableOpacity 
              style={[styles.sendBtn, { backgroundColor: isInputEmpty ? '#E0E0E0' : '#000' }]} 
              onPress={sendMessage}
              disabled={loading || isInputEmpty}
            >
              {loading ? <ActivityIndicator size="small" color="#FFF" /> : <View style={styles.whiteCircle} />}
            </TouchableOpacity>
          </View>
        </View>
      </KeyboardAvoidingView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FFFFFF' },
  header: { 
    paddingTop: 20, 
    paddingBottom: 15, 
    alignItems: 'center', 
    backgroundColor: '#FFF',
    borderBottomWidth: 0.5,
    borderBottomColor: '#EEE'
  },
  headerTitle: { fontSize: 24, fontWeight: '800', color: '#000', letterSpacing: -1 },
  listContent: { paddingHorizontal: 25, paddingTop: 10, paddingBottom: 20 },
  messageRow: { marginBottom: 28 },
  label: { fontSize: 10, fontWeight: '700', color: '#888', marginBottom: 4, textTransform: 'uppercase' },
  messageText: { fontSize: 17, color: '#1A1A1A', lineHeight: 24 },
  typingContainer: { paddingHorizontal: 25, paddingBottom: 10 },
  typingText: { fontSize: 12, color: '#AAA', fontStyle: 'italic' },
  footer: { paddingHorizontal: 20, paddingBottom: Platform.OS === 'ios' ? 20 : 30, paddingTop: 10, backgroundColor: '#FFF' },
  inputWrapper: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#F5F5F5',
    borderRadius: 30,
    paddingHorizontal: 20,
    height: 55,
  },
  input: { flex: 1, fontSize: 16, color: '#000' },
  sendBtn: { width: 44, height: 44, borderRadius: 22, justifyContent: 'center', alignItems: 'center', marginLeft: 10 },
  whiteCircle: { width: 16, height: 16, borderRadius: 8, backgroundColor: '#FFF' }
});