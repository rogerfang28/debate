### 📁 `components/auth/` — Authentication UI Components

This folder contains all components related to the authentication flow for the DebateGraph app, including login, signup, logout, and shared UI elements.

---

### 📦 Components Overview

| File               | Purpose                                                             |
| ------------------ | ------------------------------------------------------------------- |
| `AuthForm.jsx`     | **Main entry point** that toggles between login and signup forms    |
| `LoginForm.jsx`    | Handles user login using `identifier` (username/email) and password |
| `SignupForm.jsx`   | Handles user registration using email, username, and password       |
| `LogoutButton.jsx` | Reusable button to clear token and trigger logout                   |
| `AuthInput.jsx`    | Reusable form input component (label + input + styling)             |
| `AuthErrorBox.jsx` | Displays error messages inside login/signup forms                   |

---

### 🧠 Design Philosophy

* `AuthForm` handles **routing and mode switching**
* `LoginForm` and `SignupForm` handle their own **form logic and submission**
* `AuthInput` and `AuthErrorBox` are **pure presentational components**
* Each form is kept **small, readable, and testable**

---

### 🚀 How to Use

```jsx
import AuthForm from '@/components/auth/AuthForm';

<AuthForm onAuthSuccess={(token) => {
  // Save token or navigate
}} />
```

To display a logout button:

```jsx
import LogoutButton from '@/components/auth/LogoutButton';

<LogoutButton onLogout={() => {
  // Handle logout logic (e.g. redirect to login)
}} />
```

---

### 🛠️ Future Ideas

* Add `ForgotPasswordForm`
* Integrate OAuth (e.g. Google Sign-In)
* Support persistent login (Remember Me)
* Move `AuthInput` to a global `components/ui/` if reused
