export default async function submitAddFriend(req: any, eventData: Record<string, any>) {
    try {
        console.log("🎯 Handling submitAddFriend with data:", eventData);
        
        // Extract friend email from event data
        const friendEmail = eventData.friendEmail;
        
        if (!friendEmail) {
            console.error("❌ No friend email provided");
            return false;
        }
        
        console.log("📧 Adding friend with email:", friendEmail);
        
        try {
            // First, search for the user by email
            const { searchUsers } = await import("../../../old_server/controllers/friend/searchUsers.js");
            
            let searchResult: any = null;
            let searchError: any = null;
            
            const searchReq = {
                query: { query: friendEmail },
                user: { userId: "687ac6a3b292e5fa181ed2f1" } // TODO: Get from actual auth session
            };
            
            const searchRes = {
                status: (code: number) => ({
                    json: (data: any) => {
                        searchError = { code, ...data };
                        return searchRes;
                    }
                }),
                json: (data: any) => {
                    searchResult = data;
                    return searchRes;
                }
            };
            
            await searchUsers(searchReq, searchRes);
            
            if (searchResult && searchResult.users && searchResult.users.length > 0) {
                const targetUser = searchResult.users.find((user: any) => user.email === friendEmail);
                
                if (targetUser) {
                    // Send friend request to the found user
                    const { sendFriendRequest } = await import("../../../old_server/controllers/friend/sendFriendRequest.js");
                    
                    let requestResult: any = null;
                    let requestError: any = null;
                    
                    const requestReq = {
                        params: { userId: targetUser._id },
                        user: { userId: "687ac6a3b292e5fa181ed2f1" } // TODO: Get from actual auth session
                    };
                    
                    const requestRes = {
                        status: (code: number) => ({
                            json: (data: any) => {
                                requestError = { code, ...data };
                                return requestRes;
                            }
                        }),
                        json: (data: any) => {
                            requestResult = data;
                            return requestRes;
                        }
                    };
                    
                    await sendFriendRequest(requestReq, requestRes);
                    
                    if (requestResult) {
                        console.log("✅ Friend request sent successfully to:", targetUser.email);
                    } else if (requestError) {
                        console.log("❌ Failed to send friend request:", requestError.message);
                    }
                } else {
                    console.log("❌ No user found with email:", friendEmail);
                }
            } else if (searchError) {
                console.log("❌ Error searching for user:", searchError.message);
            } else {
                console.log("❌ No user found with email:", friendEmail);
            }
            
        } catch (error) {
            console.error("❌ Error in friend request process:", error);
        }
        
        // Close the modal after attempting to send request
        const { default: closeFriendModal } = await import("./closeFriendModal.ts");
        await closeFriendModal(req);
        
        return true;
        
    } catch (err) {
        console.error("❌ Error submitting add friend:", err);
        return false;
    }
}
