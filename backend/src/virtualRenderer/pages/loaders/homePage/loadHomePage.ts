import homePage from "../../schemas/homePage.js";
import getRooms from "./functions/getRooms.js";

export default async function loadHomePage(req: any) {
  let page = homePage();
  
  // Check if user is authenticated before trying to get rooms
  if (req.user && req.user.userId) {
    // in the page for the rooms, we need to get the user's room from the database, then we need to add it to the page
    page.userRoom = await getRooms(req.user.userId);
  } else {
    // User not authenticated, return empty rooms array
    page.userRoom = [];
  }
  
  return page;
}