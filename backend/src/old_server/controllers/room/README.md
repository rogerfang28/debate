# Room Controller

Controllers for managing rooms (chat, debate, meeting spaces) in the backend API.

## Key Features

- Create, update, and delete rooms
- Manage membership (join, leave, invite)
- Room settings & permissions
- List and view room details

## Endpoints

- `POST /room/create` — Create room
- `PATCH /room/update` — Update room
- `DELETE /room/delete` — Delete room
- `POST /room/join` — Join room
- `POST /room/leave` — Leave room
- `GET /room/list` — List rooms
- `GET /room/{id}` — Room details

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
