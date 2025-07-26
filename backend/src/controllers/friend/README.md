# Friend Controller

Controllers for managing friendships, friend requests, chat, and invitations.

## Key Features

- Send, accept, decline, and remove friend requests
- List friends and pending requests
- Search users to add as friends
- Send messages and chat with friends
- Send room invitations to friends

## Endpoints

- `POST /friend/request/:userId` — Send friend request
- `POST /friend/accept/:requestId` — Accept friend request
- `POST /friend/decline/:requestId` — Decline friend request
- `DELETE /friend/remove/:friendshipId` — Remove friend
- `GET /friend/list` — List friends and requests
- `GET /friend/search?query=...` — Search users
- `POST /friend/message/:friendId` — Send message
- `GET /friend/chat/:friendId` — Get chat history
- `POST /friend/invite/:friendId/:roomId` — Send room invitation

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
