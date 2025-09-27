import setCurrentPage from "../../virtualRenderer/utils/setPage.ts";
import loadLogInPage from "../../virtualRenderer/pages/loaders/authPages/loadLogInPage.ts";

export default async function goSignup(req: any){
    let loginPage = await loadLogInPage();
    setCurrentPage(req, loginPage);
}