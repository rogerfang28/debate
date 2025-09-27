import setCurrentPage from "../../virtualRenderer/utils/setPage.ts";
import loadSignUpPage from "../../virtualRenderer/pages/loaders/authPages/loadSignUpPage.ts";

export default async function goSignup(req: any){
    let signupPage = await loadSignUpPage();
    setCurrentPage(req, signupPage);
}