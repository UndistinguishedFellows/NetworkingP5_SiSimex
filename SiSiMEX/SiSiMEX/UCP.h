#pragma once
#include "Agent.h"

// Forward declaration
class MCP;
using MCPPtr = std::shared_ptr<MCP>;

class UCP :
	public Agent
{
public:

	// Constructor and destructor
	UCP(Node *node, uint16_t requestedItemId, const AgentLocation &uccLoc, int layer);
	~UCP();

	void update() override;
	void finalize() override;

	void OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream) override;

	// Whether or not the negotiation finished
	bool negotiationFinished() const;

	// Whether ir not there was a negotiation agreement
	bool negotiationAgreement() const;

private:

	void requestItem();
	void createChildMCP(uint16_t constraintItemId);
	void destroyChildMCP();

	void endNegotiation(TCPSocketPtr& socket, const PacketHeader& pckHeader, bool agreement);
	void endNegotiation(bool agreement);

	uint16_t _requestedItemId; /**< The item to request. */
	AgentLocation _uccLocation; /**< Location of the remote UCC agent. */
	int layer;

	MCPPtr _mcp; /**< The child MCP. */

	bool _negotiationAgreement; /**< Was there a negotiation agreement? */
};

