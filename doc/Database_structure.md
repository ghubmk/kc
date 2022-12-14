# A quick overview of the Kopano Core database structure

## Overview

The Database has the following tables:

```sql
+--------------------+
| Tables_in_kopano   |
+--------------------+
| abchanges          |
| acl                |
| changes            |
| deferredupdate     |
| hierarchy          |
| indexedproperties  |
| lob                |
| mvproperties       |
| names              |
| object             |
| objectmvproperty   |
| objectproperty     |
| objectrelation     |
| outgoingqueue      |
| properties         |
| receivefolder      |
| searchresults      |
| settings           |
| singleinstances    |
| stores             |
| syncedmessages     |
| syncs              |
| tproperties        |
| users              |
| versions           |
+--------------------+
```

## abchanges

Has a journal of all GAB changes -> every change (add/delete/modify) is in this
table.

## acl

List of all acls - each record is an ACE (access control entry) userx has
permissiony on folderz (With a pair record for rights for grant and deny, but
deny was never implemented in the Kopano server) Columns:

* id:
* hierarchy_id:
* type: 2=grant,1=deny
* rights: bitmask of the rights (owner, etc.) - same in mapi defined permissions

## changes

Has a journal of all mailbox changes -> messages in folders added, deleted,
modified - exposed via ICA.

## deferredupdate

Because the collection of data for the purpose of populating the "tproperties"
table can potentially be load-intensive, this work is done lazily. So, when
properties do get changed in the "properties" table and tproperties is not to
be updated immediately (which depends on the "max_deferred_records" config
option), an entry is record in the "deferredupdate" table. Effectively, such
entries also indicates that (part of) the "tproperties" table is out of date
and should not be used for retrieving propvals.

The "deferredupdate" table has both a "folderid" and "srcfolderid" column;
these two differ only when a copy operation was used (cf. copyFolder RPC).

## hierarchy

Is the parent-child relationship table for the mailbox (IPM_SUBTREE_ROOT -> ...)

Columns

* id:
* parent:
* type: 1=mailbox, 3=folder, 5=message, 6=recipient, 7=attachment
* flags: folders:0=root folder,1=normal folder,2=search folder;
   messages:0=normal message, 64=associative message, 1024=deleted
* owner:

## indexedproperties

Mapping between from certain properties to their hierarchyid example:
PR_ENTRY_ID abcdef match to hierarchy.id 12345

## lob

Attachments chunked into 256K blocks (only in mysql-stored format)

## Names

Nothing but named properties mapping to property IDs????? property IDs are
subtracted by 0x8500.

## outgoingqueue

Two things: 1. queue that outlook polls for messages that it wants to send, 2.
queue of spooler, of messages that it wants to send

Columns:

* flags: if even then outlook queue, if odd spooler queue
* properties - the list of properties for each hierarchy.id - each value is put
  into its corresponding column type (binary, double, etc.)
* mvproperties - looks exactly the same, but it also has orderid, so there can
* be multiple records for the same property for the same hierarchy.id (done for
  performance reasons)

## receivefolder

It points at which folder is your actual receivefolder in your hierarchy

## searchresults

When you do a search all of the results are saved and updated forever until you
delete the search folder settings - random server-wide settings

## singleinstances

Record of attachment-deduplication (single instance store)

## stores

links stores to users (hierarchy.id to users.id)

## syncedmessages

Is used in restricted ICS (when we cannot use the journal) - sync only the last 4
weeks is an example. This represents the messages that have been sent to a
mobile. only for z-push

## syncs

A list of all folders that are being tracked for ICS - we do not really use it,
we only write to it (investigation, maybe?)

## tproperties

The "properties" table is primary-indexed by hierarchyid (for InnoDB, the PI
defines the order on disk within an InnoDB block). Because mails in a folder
may be created over a long time and thus have vastly spread out hierarchyids,
reading a set of properties from all such mails is a potentially seek-intensive
disk operation.

To that end, the "tproperties" table ("tabled properties") replicates the
property data, but is primary-indexed by folderid instead. This is primarily
meant for use by tabular access, i.e. GetHierarchyTable and GetContentsTable in
MAPI. Values are also truncated to 255 bytes or characters, as this is
sufficient for the purposes of tabular access, and MAPI specifies that
truncation is allowed to happen.

The PR_RTF_COMPRESSED and PR_HTML properties are not represented in
"tproperties" at all. Requesting them via tabular access always yields
PROP_TAG(x, PT_ERROR)+MAPI_E_NOT_FOUND in Exchange and Kopano.

## users

Users table with the relation column (externid)

## versions

List of all Kopano versions ever installed on the system

## Tables specific for DB plugin

* object - everything normally in ldap
* objectmvproperty - everything normally in ldap
* objectproperty - everything normally in ldap
* objectrelation - everything normally in ldap
