# Import the libraries
import matplotlib.pyplot as plt
import pandas as pd
import torch
import os
from dataset import SentimentAnalysisDataset
from model import Model
from torch import cuda
from torch.utils.data import DataLoader
from transformers import RobertaTokenizer
from train import train

# Set up the device for GPU usage
print("=== Run on GPU ===") if cuda.is_available() else print("=== Run on CPU ===")

device = 'cuda' if cuda.is_available() else 'cpu'
cuda.empty_cache()

# Load Data
TRAIN_PATH = "data/train.csv"
VALID_PATH = "data/valid.csv"

train_df = pd.read_csv(TRAIN_PATH)
valid_df = pd.read_csv(VALID_PATH)

# Define key variables in training
# === Submit
# MAX_LEN = 256
# TRAIN_BATCH_SIZE = 16
# VALID_BATCH_SIZE = 16
# LEARNING_RATE = 1e-03
# EPOCHS = 1
# === Submit

MAX_LEN = 256
TRAIN_BATCH_SIZE = 16
VALID_BATCH_SIZE = 16
LEARNING_RATE = 1e-05
EPOCHS = 10

tokenizer = RobertaTokenizer.from_pretrained('roberta-base', truncation=True, do_lower_case=True)

train_dataset = SentimentAnalysisDataset(train_df, tokenizer, MAX_LEN)
valid_dataset = SentimentAnalysisDataset(valid_df, tokenizer, MAX_LEN)

train_loader = DataLoader(train_dataset, batch_size=TRAIN_BATCH_SIZE, shuffle=True, num_workers=0)
valid_loader = DataLoader(valid_dataset, batch_size=VALID_BATCH_SIZE, shuffle=True, num_workers=0)

model = Model().to(device)
optimizer = torch.optim.Adam(model.parameters(), lr=LEARNING_RATE)
criterion = torch.nn.CrossEntropyLoss()

print("=== Start Training ===")
print("train size:", len(train_df))
print("valid size:", len(valid_df))
train_acc, val_acc, train_loss, val_loss = train(model, device, criterion, optimizer, train_loader, valid_loader, EPOCHS)
print("=== Finish Training ===")

print("=== Start save model ===")
MODEL_NAME = f"{val_acc[-1]:.4f}".replace(".", "_")
path = os.path.join("./", MODEL_NAME)
if not os.path.exists(path): os.mkdir(path)
# torch.save(model.state_dict(), os.path.join(path, "model.pth"))
print("=== Finish save model ===")

print("=== Start plot ===")
figure, (axis1, axis2) = plt.subplots(2, 1, figsize=(10, 6))
figure.subplots_adjust(hspace=0.5)
# figure.suptitle(f'')

# For Train Acc and Val Acc
axis1.plot(train_acc, label='Train Acc')
axis1.plot(val_acc, label='Val Acc')
axis1.legend(loc='lower right')
axis1.set_title(f'Train Acc {train_acc[-1]:.4f}, Valid Acc {val_acc[-1]:.4f}')
axis1.set_xlabel("epoch")
axis1.set_ylabel("accuracy")

# For Train Loss
axis2.plot(train_loss, label='Train Loss') 
axis2.plot(val_loss, label='Val Loss') 
axis2.legend(loc='lower right')
axis2.set_title(f'Train Loss {train_loss[-1]:.4f}, Valid Loss {val_loss[-1]:.4f}')
axis2.set_xlabel("epoch")
axis2.set_ylabel("loss")

plt.savefig(os.path.join(path, "img.jpg"))
print("=== Finish plot ===")
