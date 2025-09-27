# Auth Controller

Controllers for user authentication and profile management.

## Key Features

- User signup and login
- User logout
- Update user profile (username)
- Token-based authentication (JWT)

## Endpoints

- `POST /auth/signup` — Register new user
- `POST /auth/login` — Authenticate and return token
- `POST /auth/logout` — Log out user
- `PATCH /auth/profile` — Update username

## Security & Validation

- Input sanitized & validated
- Access control enforced
- User-friendly error messages

## Coding Standards

- PHP 8.1+, strict types, PSR-12
- Dependency injection
- Consistent exception handling

## Accessibility & Compliance

- Follows project accessibility & security guidelines
  (see root docs for details)
> _Author: Project Team_
