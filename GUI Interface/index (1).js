// Download the helper library from https://www.twilio.com/docs/node/install
const twilio = require("twilio"); // Or, for ESM: import twilio from "twilio";
const express = require("express");
const bodyParser = require("body-parser");
const { Client } = require("pg");
const { parse } = require("dotenv");
const bcrypt =  require("bcrypt");
const env = require("dotenv")
const session = require("express-session");
const passport = require("passport");
const { Strategy } = require("passport-local");
const GoogleStrategy = require("passport-google-oauth2");
env.config();

const app = express();
const port = 3000;
app.set('view engine', 'ejs');

app.use(express.static("public"));
app.use(bodyParser.urlencoded({ extended: true }));

app.use(
  session({
  secret: process.env.SESSION_SECRET,
  resave: false,
  saveUninitialized: true,
  cookie:
  {
    maxAge: 1000 * 60 * 60,
  },
}));


app.use(passport.initialize());
app.use(passport.session());

// Find your Account SID and Auth Token at twilio.com/console
// and set the environment variables. See http://twil.io/secure
const accountSid = process.env.TWILIO_ACCOUNT_SID;
const authToken = process.env.TWILIO_AUTH_TOKEN;
const client = twilio(accountSid, authToken);

const saltRounds = 10;
// initializing CockroachDB client
const dbClient = new Client({
  connectionString: process.env.COCKROACHDB_URL,
});

const dbClient1 = new Client({
  connectionString: process.env.COCKROACHDB_URL2,
});

async function connectToDatabase() {
  try {
    await dbClient.connect();
    console.log("Connected to CockroachDB");
  } catch (err) {
    console.error("Database connection error", err.stack);
  }
}
async function connectToUsers() {
  try {
    await dbClient1.connect();
    console.log("Connected to Users Database");
  } catch (err) {
    console.error("Database connection error", err.stack);
  }
}

async function insertDataIntoDB(field1, field2, field3 ) {
  try{
    const query = 'INSERT INTO gas_analyser.sensor_data (field1, field2, field3 ) VALUES ($1, $2, $3 )';
    await dbClient.query(query, [parseFloat(field1), parseFloat(field2), parseFloat(field3)]);
    console.log("Data inserted into DB");
  }
  catch(err){
    console.error("Error inserting data into DB", err.stack);
  }
}

async function fetchAndStoreData() {
  try {
    const response = await fetch("https://api.thingspeak.com/channels/2687973/feeds/last.json?api_key=B58ZP1GJYB2UP2GS");

    // Check if the response was successful
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const data = await response.json();
    
    // Extract fields from the ThingSpeak response
    const { field1, field2, field3} = data;

    if (field1 && field2 && field3 ) {
      // Store the data in the database
      await insertDataIntoDB(field1, field2, field3 );
      console.log("Data successfully stored:", { field1, field2, field3 });
      if(parseFloat(field1) > 200 || parseFloat(field2) > 400 || parseFloat(field3) > 1000)
      {
        await sendMessage('+917973389551')
        console.log("Message sent successfully");
      }
    } else {
      console.log("Incomplete data received from ThingSpeak");
    }

  } catch (error) {
    console.error("Error fetching data from ThingSpeak:", error.message);
  }
}

setInterval(fetchAndStoreData, 10000);

function getphoneNumber(phoneNumber)
{
  if(phoneNumber.startsWith("+91") && phoneNumber.length == 13)
  {
    return phoneNumber;
  }
  else if(phoneNumber.startsWith("91") && phoneNumber.length == 12)
  {
    return `+${phoneNumber}`;
  }
  else if(phoneNumber.length == 10)
  {
    return `+91${phoneNumber}`;
  }
  else
  {
    return '0';
  }
}

const sendMessage = async (to) => {
  try {
    const message = await client.messages.create({
      body: "Alert! Gas Leakage Detected",
      from: 'whatsapp:+14155238886',
      to: `whatsapp:${to}`
    });
    console.log("Message sent to:", message.to);
  } catch (error) {
    console.log("Failed to send message:", error.message);
  }
};


app.get("/secrets", async (req, res) => {
  try {
    // Fetch data from ThingSpeak API
    const response = await fetch("https://api.thingspeak.com/channels/2687973/fields/1/last.json?api_key=B58ZP1GJYB2UP2GS");
    const data = await response.json();
    
    // Render the index.ejs and pass data to it
    res.render("index", { sensorData: data });
  } catch (error) {
    console.error("Error fetching data from ThingSpeak:", error);
    res.render("index", { sensorData: null });
  }
});

app.get("/mq2_reading", async(req, res) => {
  try {
    // Fetch data from ThingSpeak API
    const response = await fetch("https://api.thingspeak.com/channels/2687973/fields/1/last.json?api_key=B58ZP1GJYB2UP2GS");
    const data = await response.json();
    
    // Render the index.ejs and pass data to it
    res.render("mq2_reading", { sensorData: data });
  } catch (error) {
    console.error("Error fetching data from ThingSpeak:", error);
    res.render("mq2_reading", { sensorData: null });
  }
});

app.post("/send-message", async (req, res) => {  
  try {
    await sendMessage('+917973389551');
    res.redirect("/");
  } catch (error) {
    res.send("Failed to send message");
  }
});

app.get("/logout", (req, res) => {
  req.logout(function(err){
    if(err){
      console.log(err);
    }
    else
    {
      res.redirect("/");
    }
  });
});

app.get(
  "/auth/google", 
  passport.authenticate("google", 
  { 
    scope: ["profile", "email"] 
  })
);
app.get("/auth/google/secrets", 
  passport.authenticate("google",
  {
    successRedirect: "/secrets",
    failureRedirect: "/login"
  })
)

app.get("/secrets", (req, res) => {
  console.log("User:", req.user);
  if (req.isAuthenticated()) {
    res.render("index.ejs");
  }
  else {
    res.redirect("/login2");
  }
});
app.get("/login", (req, res) => {
  res.render("login2.ejs");
});

app.get("/" , (req, res) => {
  res.redirect("/register");
});

app.get("/register", (req, res) => {
  res.render("register2.ejs");
});

app.post("/register", async (req, res) => {
  const username = req.body.username;
  const email = req.body.email;
  const password = req.body.password;

  console.log("Username:", username);
  console.log("Email:", email);
  console.log("Password:",password);
  try {
    const query = "SELECT * FROM users WHERE email = $1";
    const checkResult = await dbClient1.query(query, [email]
    );
    console.log("Check result:", checkResult.rows);
    if (checkResult.rows.length > 0) 
    {
      res.send("Email already exists. Try logging in.");
    } 
    else 
    {
      // hash the password
      bcrypt.hash(password, saltRounds, async (err, hash) => {
        if (err) {
          console.error("Error hashing the password", err);
        }
        else {
          console.log("Hashed password:", hash);
          const query = "INSERT INTO users (username, email, password) VALUES ($1, $2, $3)";
          const result = await dbClient1.query(query,
            [username, email, hash]
          );
          res.render("login2.ejs");
        }
      });
    }
  }
  catch (error) {
    res.send("Failed to register");
  }
});

app.post("/login", passport.authenticate("local", {
  successRedirect: "/secrets",
  failureRedirect: "/login",
}));


connectToDatabase();
connectToUsers();

passport.use(
  "local",
  
  new Strategy(
    { usernameField: 'email', passwordField: 'password', passReqToCallback: true },
    async function verify(req, email, password, done){
    const username = req.body.username;
    try
    {
      const query = "SELECT * FROM users WHERE email = $1";
      const result = await dbClient1.query(query, [email]);
      console.log("Email:", email);
      console.log("Password: ", password);
      console.log("Username:", username);
      console.log("Result:", result.rows);
        if (result.rows.length > 0) {
          const user = result.rows[0];
          const hashedPassword = user.password;
          console.log("Hashed password:", hashedPassword);
          bcrypt.compare(password, hashedPassword, (err, valid) => {
            if(err){
              return done(err);
            }else{
              if (valid) {
                console.log("User authenticated");
                return done(null, user);
              } else {
                return done(null, false);
              }
            }
          });
        } else {
          return done(null, false);
        }
    }catch(error){
      console.log(error);
    }
  }
));

passport.use("google",
  new GoogleStrategy({
    clientID: process.env.GOOGLE_CLIENT_ID,
    clientSecret: process.env.GOOGLE_CLIENT_SECRET,
    callbackURL: "http://localhost:3000/auth/google/secrets",
    userProfileURL: "https://www.googleapis.com/oauth2/v1/certs",
  },
  async function (accessToken, refreshToken, profile, done) {
    try{
      console.log("Profile:", profile);
      const query = "SELECT * FROM users WHERE email = $1";
      const result = await dbClient1.query(query, [profile.email]);
      if(result.rows.length === 0){
        const newUser = await dbClient1.query("INSERT INTO users (username, email, password) VALUES ($1, $2, $3) RETURNING *", [profile.displayName, profile.email, "google"]);
        return done(null, newUser.rows[0]);
      }
      else{
        return done(null, result.rows[0]);
      }
    }
    catch(error){
      console.log(error);
    }
  })
);

passport.serializeUser((user, done) => {
  done(null, user);
});

passport.deserializeUser((user, done) => {
  done(null, user);
});

app.listen(port, () => {
  console.log(`Server running on port: ${port}`);
});





