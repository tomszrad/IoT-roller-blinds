const express = require('express');
const fs = require('fs');
const path = require('path');
const mqtt = require('mqtt');

// Funkcja do obliczania liczby kroków

const pelenObrot = _NUMBER_OF_STEPS_FOR_CLOSING_THE_ROLLERBLIND;

//obecnie zapytania do mqtt z zewnatrz robią mismatch ze stany.json. przemyśl zrobienie tak, aby serwer express subskrybowal mqtt i aktualizowal procent na wypadek ewentualnych publikacji na mqtt z innych zrodel


function obliczLiczbeKrokow(kierunek, wspolrzedna, cel) {
  let liczbaKrokow = (cel - wspolrzedna) * pelenObrot / 100;
  if (kierunek) {
    liczbaKrokow *= -1;
  }
  return Math.round(liczbaKrokow);
}

const app = express();
const port = 1882;


// Middleware dla autoryzacji klucza API
const apiKeyMiddleware = (req, res, next) => {
  // Sprawdzenie, czy zapytanie pochodzi z localhosta
  const isLocalhost = req.hostname === 'localhost' || req.ip === '127.0.0.1';
  
  if (isLocalhost) {
    // Jeśli zapytanie pochodzi z localhosta, pomiń autoryzację klucza API
    return next();
  }

  // Inaczej sprawdź klucz API
  const apiKey = req.headers['x-api-key'];
  if (apiKey && apiKey === '_API_AUTHORIZATION_KEY') {
    next(); // Klucz jest poprawny, przejdź do następnego middleware lub trasy
  } else {
    res.status(401).json({ message: 'Nieautoryzowany' }); // Klucz jest niepoprawny, zwróć błąd
  }
};

// Używaj middleware dla wszystkich tras
app.use(apiKeyMiddleware);

// Konfiguracja klienta MQTT
const mqttOptions = {
  host: 'localhost',
  port: 1883,
  username: '_MQTT_USERNAME',
  password: '_MQTT_PASSWORD'
};
const client = mqtt.connect(mqttOptions);

client.on('connect', () => {
  console.log('Connected to MQTT broker');
  client.subscribe('reached', (err) => {
    if (err) {
      console.error('Error subscribing to topic reached:', err);
    } else {
      console.log('Subscribed to topic reached');
    }
  });
});

client.on('message', (topic, message) => {
  if (topic === 'reached') {
    const filePath = path.join(__dirname, 'stany.json');
    fs.readFile(filePath, 'utf8', (err, data) => {
      if (err) {
        console.error('Error reading the file:', err);
        return;
      }

      const stany = JSON.parse(data);
      stany.czysierusza = false;

      fs.writeFile(filePath, JSON.stringify(stany, null, 2), 'utf8', (err) => {
        if (err) {
          console.error('Error writing to the file:', err);
        } else {
          console.log('Updated czysierusza to false in stany.json');
        }
      });
    });
  }
});

app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.get('/wspolrzedne', (req, res) => {
  const filePath = path.join(__dirname, 'stany.json');

  fs.readFile(filePath, 'utf8', (err, data) => {
    if (err) {
      res.status(500).send('Error reading the file');
      return;
    }

    const stany = JSON.parse(data);

    if (stany.czysierusza) {
      res.json(stany);
      return;
    }

    let { w1, w2, w3 } = req.query;

    let isUpdated = false;

    if (w1 !== undefined) {
      const kierunek = _R1_DIRECTION_BOOLEAN; // Ustawiamy kierunek na true
      const wspolrzedna = stany.roleta1.wspolrzedna;
      const cel = parseInt(w1, 10);
      stany.roleta1.wspolrzedna = cel; // Aktualizujemy współrzędną w stany.json
      const liczbaKrokow = obliczLiczbeKrokow(kierunek, wspolrzedna, cel);
      w1 = liczbaKrokow.toString(); // Przygotowanie wartości do wysłania do MQTT
      isUpdated = true;
    }
    if (w2 !== undefined) {
      const kierunek = _R2_DIRECTION_BOOLEAN;
      const wspolrzedna = stany.roleta2.wspolrzedna;
      const cel = parseInt(w2, 10);
      stany.roleta2.wspolrzedna = cel;
      const liczbaKrokow = obliczLiczbeKrokow(kierunek, wspolrzedna, cel);
      w2 = liczbaKrokow.toString();
      isUpdated = true;
    }
    if (w3 !== undefined) {
      const kierunek = _R3_DIRECTION_BOOLEAN;
      const wspolrzedna = stany.roleta3.wspolrzedna;
      const cel = parseInt(w3, 10);
      stany.roleta3.wspolrzedna = cel;
      const liczbaKrokow = obliczLiczbeKrokow(kierunek, wspolrzedna, cel);
      w3 = liczbaKrokow.toString();
      isUpdated = true;
    }

    if (isUpdated) {
        stany.czysierusza = true;
        fs.writeFile(filePath, JSON.stringify(stany, null, 2), 'utf8', (err) => {
          if (err) {
            res.status(500).send('Error writing to the file');
            return;
          }
      
          // Prepare the message array for MQTT
          const message = [parseInt(w1), parseInt(w2), parseInt(w3)];
      
          // Publish message to MQTT
          client.publish('rollerblinds', JSON.stringify(message), (err) => {
            if (err) {
              console.error('Error publishing to MQTT:', err);
              res.status(500).send('Error publishing to MQTT');
              return;
            }
      
            res.json(stany);
          });
        });
      } else {
        res.json(stany);
      }
      
  });
});

app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
