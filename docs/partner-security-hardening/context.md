# Evidence context

Revision inspected: develop at ee225bd. Source drift: present only as CRLF working-tree conversion; ignore-CR diff was empty before this work.

| ID | Evidence | Relevant source |
| --- | --- | --- |
| E1 | Restaurant writes have no authentication or ownership check | services/RestaurantService/src/main.cpp |
| E2 | JWT identifies a customer user but carries no partner capability | common/src/JwtManager.cpp |
| E3 | Gateway proxies public restaurant POST, PUT and DELETE | services/ApiGateway/src/main.cpp |
| E4 | Restaurant schema has no owner, membership, lifecycle or version | common/src/Database.cpp |

This is source-derived design evidence, not a completed security scan.
