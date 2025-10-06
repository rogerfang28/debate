import setCurrentPage from "../../virtualRenderer/utils/setPage";
import { loadHomePage } from "../../virtualRenderer/pages/loaders";

export default async function goHome(req: any){
    let homePage = await loadHomePage();
    setCurrentPage(req, homePage);
}