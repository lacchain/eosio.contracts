<h1 class="contract">activate</h1>

---
spec_version: "0.2.0"
title: Activate Protocol Feature
summary: 'Activate protocol feature {{nowrap feature_digest}}'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

{{$action.account}} activates the protocol feature with a digest of {{feature_digest}}.

<h1 class="contract">canceldelay</h1>

---
spec_version: "0.2.0"
title: Cancel Delayed Transaction
summary: '{{nowrap canceling_auth.actor}} cancels a delayed transaction'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

{{canceling_auth.actor}} cancels the delayed transaction with id {{trx_id}}.

<h1 class="contract">deleteauth</h1>

---
spec_version: "0.2.0"
title: Delete Account Permission
summary: 'Delete the {{nowrap permission}} permission of {{nowrap account}}'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

Delete the {{permission}} permission of {{account}}.

<h1 class="contract">linkauth</h1>

---
spec_version: "0.2.0"
title: Link Action to Permission
summary: '{{nowrap account}} sets the minimum required permission for the {{#if type}}{{nowrap type}} action of the{{/if}} {{nowrap code}} contract to {{nowrap requirement}}'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

{{account}} sets the minimum required permission for the {{#if type}}{{type}} action of the{{/if}} {{code}} contract to {{requirement}}.

{{#if type}}{{else}}Any links explicitly associated to specific actions of {{code}} will take precedence.{{/if}}

<h1 class="contract">newaccount</h1>

---
spec_version: "0.2.0"
title: Create New Account
summary: '{{nowrap creator}} creates a new account with the name {{nowrap name}}'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

{{creator}} creates a new account with the name {{name}} and the following permissions:

owner permission with authority:
{{to_json owner}}

active permission with authority:
{{to_json active}}

<h1 class="contract">reqactivated</h1>

---
spec_version: "0.2.0"
title: Assert Protocol Feature Activation
summary: 'Assert that protocol feature {{nowrap feature_digest}} has been activated'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Assert that the protocol feature with a digest of {{feature_digest}} has been activated.

<h1 class="contract">reqauth</h1>

---
spec_version: "0.2.0"
title: Assert Authorization
summary: 'Assert that authorization by {{nowrap from}} is provided'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

Assert that authorization by {{from}} is provided.

<h1 class="contract">setabi</h1>

---
spec_version: "0.2.0"
title: Deploy Contract ABI
summary: 'Deploy contract ABI on account {{nowrap account}}'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

Deploy the ABI file associated with the contract on account {{account}}.

<h1 class="contract">setalimits</h1>

---
spec_version: "0.2.0"
title: Adjust Resource Limits of Account
summary: 'Adjust resource limits of account {{nowrap account}}'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

{{$action.account}} updates {{account}}’s resource limits to have a RAM quota of {{ram_bytes}} bytes, a NET bandwidth quota of {{net_weight}} and a CPU bandwidth quota of {{cpu_weight}}.

<h1 class="contract">setcode</h1>

---
spec_version: "0.2.0"
title: Deploy Contract Code
summary: 'Deploy contract code on account {{nowrap account}}'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

Deploy compiled contract code to the account {{account}}.

<h1 class="contract">setparams</h1>

---
spec_version: "0.2.0"
title: Set System Parameters
summary: 'Set system parameters'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

{{$action.account}} sets system parameters to:
{{to_json params}}

<h1 class="contract">setpriv</h1>

---
spec_version: "0.2.0"
title: Make an Account Privileged or Unprivileged
summary: '{{#if is_priv}}Make {{nowrap account}} privileged{{else}}Remove privileged status of {{nowrap account}}{{/if}}'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

{{#if is_priv}}
{{$action.account}} makes {{account}} privileged.
{{else}}
{{$action.account}} removes privileged status of {{account}}.
{{/if}}

<h1 class="contract">setprods</h1>

---
spec_version: "0.2.0"
title: Set Block Producers
summary: 'Set block producer schedule'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

{{$action.account}} proposes a block producer schedule of:
{{#each schedule}}
  1. {{this.producer_name}}
{{/each}}

The block signing authorities of each of the producers in the above schedule are listed below:
{{#each schedule}}
### {{this.producer_name}}
{{to_json this.authority}}
{{/each}}

<h1 class="contract">unlinkauth</h1>

---
spec_version: "0.2.0"
title: Unlink Action from Permission
summary: '{{nowrap account}} unsets the minimum required permission for the {{#if type}}{{nowrap type}} action of the{{/if}} {{nowrap code}} contract'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

{{account}} removes the association between the {{#if type}}{{type}} action of the{{/if}} {{code}} contract and its minimum required permission.

{{#if type}}{{else}}This will not remove any links explicitly associated to specific actions of {{code}}.{{/if}}

<h1 class="contract">updateauth</h1>

---
spec_version: "0.2.0"
title: Modify Account Permission
summary: 'Add or update the {{nowrap permission}} permission of {{nowrap account}}'
icon: @ICON_BASE_URL@/@ACCOUNT_ICON_URI@
---

Modify, and create if necessary, the {{permission}} permission of {{account}} to have a parent permission of {{parent}} and the following authority:
{{to_json auth}}

<h1 class="contract">onerror</h1>

---
spec_version: "0.2.0"
title: On Error Action
summary: 'This action is delivered to the {{nowrap sender}} of a deferred transaction'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

On error action, notification of this action is delivered to the {{sender}} of a deferred transaction {{sent_trx}} when an objective error occurs while executing the deferred transaction.

<h1 class="contract">addentity</h1>

---
spec_version: "0.2.0"
title: Add Entity
summary: 'Register a partner or non-partner {{nowrap entity_name}} entity on the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows the user to register a partner or non-partner {{nowrap entity_name}} entity on the network specifying its {{entity_type}} type and {{pubkey}} public key.

<h1 class="contract">rmentity</h1>

---
spec_version: "0.2.0"
title: Remove Entity
summary: 'Remove a partner or non-partner {{nowrap entity_name}} entity from the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows the user to remove a partner or non-partner {{entity_name}} entity from the network.

<h1 class="contract">addvalidator</h1>

---
spec_version: "0.2.0"
title: Register Validator
summary: 'Register a {{nowrap name}} validator node on the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows a {{entity}} partner entity to register a new {{name}} validator node on the network with its respective {{validator_authority}} authority.

<h1 class="contract">addwriter</h1>

---
spec_version: "0.2.0"
title: Add Writer
summary: 'Register a {{nowrap name}} writer node on the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows a {{entity}} partner or non-partner entity to register a new {{name}} writer node on the network with its {writer_authority}} authority.

<h1 class="contract">addboot</h1>

---
spec_version: "0.2.0"
title: Add Boot Node
summary: 'Register a {{nowrap name}} boot node on the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows a {{entity}} partner entity to register a new {{name}} boot node on the network.

<h1 class="contract">addobserver</h1>

---
spec_version: "0.2.0"
title: Add Observer Node
summary: 'Register an {{nowrap observer}} observer node on the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows a {{name}} partner or non-partner entity to register a new {{observer}} observer node on the network.

<h1 class="contract">netaddgroup</h1>

---
spec_version: "0.2.0"
title: Create Group of Nodes
summary: 'Create a {{nowrap name}} group to organize {{nowrap nodes}} nodes on the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows the {{name}} permissioning committee to create a group for {{nodes}} nodes on the network.

<h1 class="contract">rmnode</h1>

---
spec_version: "0.2.0"
title: Remove a Node
summary: 'Remove a {{nowrap node_name}} Node from the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows an entity to remove a {{node_name}} Node from the network.

<h1 class="contract">netrmgroup</h1>

---
spec_version: "0.2.0"
title: Remove a Group of Nodes
summary: 'Remove a {{nowrap name}} Group of Nodes from the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows the permissioning committee to remove a {{name}} Group of Nodes from the network.

<h1 class="contract">netsetgroup</h1>

---
spec_version: "0.2.0"
title: Link a Node
summary: 'Link a {{nowrap node}} Node with a List of Groups of the network'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows the entity nodes to link a {{node}} Node with a {{groups}} List of Groups of the network.

<h1 class="contract">setnodeinfo</h1>

---
spec_version: "0.2.0"
title: Set Node Information
summary: 'Set information to a {{nowrap node}} Node'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows the entity to set {{info}} information to a specific {{node}} node.

<h1 class="contract">setnodexinfo</h1>

---
spec_version: "0.2.0"
title: Set Node Extended Information
summary: 'Set extended information to a {{nowrap node}} Node'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows the permissioning committee to set {{ext_info}} extended information to a specific {{node}} node.

<h1 class="contract">setentinfo</h1>

---
spec_version: "0.2.0"
title: Set Entity Information
summary: 'Set information about an {{nowrap entity}} Entity'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows an {{entity}} entity to set {{info}} information about a specific node on the network.

<h1 class="contract">setentxinfo</h1>

---
spec_version: "0.2.0"
title: Set Extended Entity Information
summary: 'Set extended information to an {{nowrap entity}} Entity'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows an {{entity}} entity to set {{ext_info}} extended information such as entity name, location, and contact information.


<h1 class="contract">setschedule</h1>

---
spec_version: "0.2.0"
title: Set Schedule Action
summary: 'Set schedule action, sets a new list of active {{nowrap validators}} Validators'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Set schedule action, sets a new list of active {{validators}} validators, by proposing a schedule change, once the block that contains the proposal becomes irreversible, the schedule is promoted to "pending" automatically. Once the block that promotes the schedule is irreversible, the schedule will become "active".

<h1 class="contract">setram</h1>

---
spec_version: "0.2.0"
title: Set RAM Resource
summary: 'Set RAM resource to an {{nowrap account}} account from an entity'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Allows the user to set {{ram_bytes}} RAM resource to an {{account}} account from an {{entity}} entity.

<h1 class="contract">onblock</h1>

---
spec_version: "0.2.0"
title: Effect Trigger
summary: 'It is used for network monitoring and as an effect trigger for each {{nowrap bh}} block'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

It is executed with each {{bh}} block and passes some data that can be used for network monitoring and as an effect trigger.

<h1 class="contract">reindex</h1>

---
spec_version: "0.2.0"
title: Dummy Action
summary: 'Dummy action'
icon: @ICON_BASE_URL@/@ADMIN_ICON_URI@
---

Dummy action for testing and development purposes.