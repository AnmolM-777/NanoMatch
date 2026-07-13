import random

def generate_csv(filepath, num_records=100000):
    random.seed(42)
    active_orders = []
    
    with open(filepath, 'w') as f:
        order_id = 1
        for i in range(num_records):
            # 10% chance to cancel an active order
            if random.random() < 0.10 and active_orders:
                cancel_id = random.choice(active_orders)
                f.write(f"C,{cancel_id}\n")
                active_orders.remove(cancel_id)
            else:
                price = random.randint(9900, 10100)
                qty = random.randint(10, 1000)
                side = 'B' if random.random() < 0.5 else 'S'
                f.write(f"A,{order_id},{price},{qty},{side}\n")
                active_orders.append(order_id)
                order_id += 1

if __name__ == "__main__":
    import os
    os.makedirs(os.path.dirname(os.path.abspath(__file__)), exist_ok=True)
    generate_csv(os.path.join(os.path.dirname(__file__), "data.csv"), 1000000)
    print("Generated data.csv with 1,000,000 events.")
