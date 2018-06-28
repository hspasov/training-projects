CREATE TABLE freefall.data_fetches (
	id int PRIMARY KEY NOT NULL,
	timestamp text NOT NULL
);

CREATE TABLE freefall.subscriptions (
	id int PRIMARY KEY NOT NULL,
	fly_from text NOT NULL,
	fly_to text NOT NULL,
	is_roundtrip int NOT NULL DEFAULT 0,
	UNIQUE(fly_from, fly_to, is_roundtrip),
	CHECK(fly_from <> fly_to)
);

CREATE TABLE freefall.routes (
	id int PRIMARY KEY NOT NULL,
	booking_token text NOT NULL UNIQUE,
	subscription_id int NOT NULL,
	data_fetch_id int NOT NULL,
	FOREIGN KEY(subscription_id) REFERENCES subscriptions(id),
	FOREIGN KEY(data_fetch_id) REFERENCES data_fetches(id)
);

CREATE TABLE freefall.airports (
	id int PRIMARY KEY NOT NULL,
	iata_code text NOT NULL UNIQUE,
	name text NOT NULL UNIQUE,
	data_fetch_id int NOT NULL,
	FOREIGN KEY(data_fetch_id) REFERENCES data_fetches(id)
);

CREATE TABLE freefall.airlines (
	id int PRIMARY KEY NOT NULL,
	name text NOT NULL UNIQUE,
	code text NOT NULL UNIQUE,
	logo_url text UNIQUE,
	data_fetch_id int NOT NULL,
	FOREIGN KEY(data_fetch_id) REFERENCES data_fetches(id) 
);

CREATE TABLE freefall.flights (
	id int PRIMARY KEY NOT NULL,
	airline_id int NOT NULL,
	airport_from_id int NOT NULL,
	airport_to_id int NOT NULL,
	dtime text NOT NULL,
	atime text NOT NULL,
	price int NOT NULL, -- stored as the lowest common unit, e.g. cents, pence, etc.
	currency text NOT NULL,
	remote_id text NOT NULL UNIQUE,
	data_fetch_id int NOT NULL,
	FOREIGN KEY (airline_id) REFERENCES airlines(id),
	FOREIGN KEY (airport_from_id) REFERENCES airports(id),
	FOREIGN KEY (airport_to_id) REFERENCES airports(id),
	FOREIGN KEY (data_fetch_id) REFERENCES data_fetches(id)   
);

CREATE TABLE freefall.routes_flights (
	id int PRIMARY KEY NOT NULL,
	route_id int NOT NULL,
	flight_id int NOT NULL,
	is_return int NOT NULL,
	data_fetch_id int NOT NULL,
	FOREIGN KEY (route_id) REFERENCES routes(id),
	FOREIGN KEY (flight_id) REFERENCES flights(id),
	FOREIGN KEY (data_fetch_id) REFERENCES data_fetches(id)
);
