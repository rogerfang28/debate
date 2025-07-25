// * Status: Complete
// This is the simpliest code of all time figure it out yourself
// Ok fine it just returns a 200 status code and a message idk man

export const logout = (req, res) => {
  // With JWT, logout is handled client-side
  res.status(200).json({ message: 'Logged out' });
};
